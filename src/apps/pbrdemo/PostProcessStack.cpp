#include "PostProcessStack.hpp"
#include "scene/lights/Light.hpp"
#include "scene/Sky.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"
#include <chrono>

PostProcessStack::PostProcessStack(const glm::vec2 & resolution){
	const int renderWidth	= int(resolution[0]);
	const int renderHeight	= int(resolution[1]);
	const Descriptor desc = {Layout::RGB16F, Filter::LINEAR_NEAREST, Wrap::CLAMP};
	_bloomBuffer	= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, desc, false));
	_toneMapBuffer 	= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, desc, false));
	_blur		= std::unique_ptr<GaussianBlur>(new GaussianBlur(_settings.bloomRadius, 2));
	_preferredFormat.push_back(desc);
	_needsDepth = false;
	_bloomProgram		   = Resources::manager().getProgram2D("bloom");
	_bloomComposite = Resources::manager().getProgram2D("scale-texture");
	_toneMappingProgram	   = Resources::manager().getProgram2D("tonemap");
	_fxaaProgram		   = Resources::manager().getProgram2D("fxaa");
	checkGLError();
}

void PostProcessStack::process(const Texture * texture, Framebuffer & framebuffer, size_t layer) {

	const glm::vec2 invRenderSize = 1.0f / glm::vec2(framebuffer.width(), framebuffer.height());

	const Texture * sceneResult = texture;
	if(_settings.bloom) {
		// --- Bloom selection pass ------
		_bloomBuffer->bind();
		_bloomBuffer->setViewport();
		_bloomProgram->use();
		_bloomProgram->uniform("luminanceTh", _settings.bloomTh);
		ScreenQuad::draw(texture);
		_bloomBuffer->unbind();
		
		// --- Bloom blur pass ------
		_blur->process(_bloomBuffer->texture(), *_bloomBuffer);
		
		// Add back the scene content.
		framebuffer.bind();
		framebuffer.setViewport();
		GLUtilities::setBlendState(true, BlendEquation::ADD, BlendFunction::ONE, BlendFunction::ONE);
		_bloomComposite->use();
		_bloomComposite->uniform("scale", _settings.bloomMix);
		ScreenQuad::draw(_bloomBuffer->texture());
		GLUtilities::setBlendState(false);
		framebuffer.unbind();
		sceneResult = framebuffer.texture();
		// Tonemapping below ensures that we will always have an intermediate target.
	}
	
	// --- Tonemapping pass ------
	_toneMapBuffer->bind();
	_toneMapBuffer->setViewport();
	_toneMappingProgram->use();
	_toneMappingProgram->uniform("customExposure", _settings.exposure);
	_toneMappingProgram->uniform("apply", _settings.tonemap);
	ScreenQuad::draw(sceneResult);
	_toneMapBuffer->unbind();

	if(_settings.fxaa) {
		framebuffer.bind(layer);
		framebuffer.setViewport();
		_fxaaProgram->use();
		_fxaaProgram->uniform("inverseScreenSize", invRenderSize);
		ScreenQuad::draw(_toneMapBuffer->texture());
		framebuffer.unbind();
	} else {
		GLUtilities::blit(*_toneMapBuffer, framebuffer, 0, layer, Filter::LINEAR);
	}

	checkGLError();
}

void PostProcessStack::updateBlurPass(){
	_blur.reset(new GaussianBlur(_settings.bloomRadius, 2));
}

void PostProcessStack::clean() {
	_blur->clean();
	_bloomBuffer->clean();
	_toneMapBuffer->clean();
}

void PostProcessStack::resize(unsigned int width, unsigned int height) {
	const glm::vec2 renderRes(width, height);
	_toneMapBuffer->resize(renderRes);
	_bloomBuffer->resize(renderRes);
	checkGLError();
}

void PostProcessStack::interface(){
	ImGui::Checkbox("Bloom  ", &_settings.bloom);
	if(_settings.bloom) {
		ImGui::SameLine(120);
		ImGui::SliderFloat("Th.##Bloom", &_settings.bloomTh, 0.5f, 2.0f);

		ImGui::PushItemWidth(80);
		if(ImGui::InputInt("Rad.##Bloom", &_settings.bloomRadius, 1, 10)) {
			_settings.bloomRadius = std::max(1, _settings.bloomRadius);
			updateBlurPass();
		}
		ImGui::PopItemWidth();
		ImGui::SameLine(120);
		ImGui::SliderFloat("Mix##Bloom", &_settings.bloomMix, 0.0f, 1.5f);
	}

	ImGui::Checkbox("Tonemap ", &_settings.tonemap);
	if(_settings.tonemap) {
		ImGui::SameLine(120);
		ImGui::SliderFloat("Exposure", &_settings.exposure, 0.1f, 10.0f);
	}
	ImGui::Checkbox("FXAA", &_settings.fxaa);
}
