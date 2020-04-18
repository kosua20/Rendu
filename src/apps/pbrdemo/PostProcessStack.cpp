#include "PostProcessStack.hpp"
#include "scene/lights/Light.hpp"
#include "scene/Sky.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"
#include <chrono>

PostProcessStack::PostProcessStack(const glm::vec2 & resolution){
	const int renderWidth	= int(resolution[0]);
	const int renderHeight	= int(resolution[1]);
	const int renderHWidth  = int(resolution[0]/2);
	const int renderHHeight = int(resolution[1]/2);
	const Descriptor desc = {Layout::RGB16F, Filter::LINEAR_NEAREST, Wrap::CLAMP};
	_bloomBuffer	= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, desc, false));
	_toneMapBuffer 	= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, desc, false));
	_blurBuffer		= std::unique_ptr<GaussianBlur>(new GaussianBlur(renderHWidth, renderHHeight, _settings.bloomRadius, Layout::RGB16F));
	_preferredFormat.push_back(desc);
	_needsDepth = false;
	_bloomProgram		   = Resources::manager().getProgram2D("bloom");
	_bloomCompositeProgram = Resources::manager().getProgram2D("bloom-composite");
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
		_blurBuffer->process(_bloomBuffer->textureId());
		
		// Draw the blurred bloom back into the scene framebuffer.
		_bloomBuffer->bind();
		_bloomBuffer->setViewport();
		_bloomCompositeProgram->use();
		_bloomCompositeProgram->uniform("mixFactor", _settings.bloomMix);
		ScreenQuad::draw({texture, _blurBuffer->textureId()});
		_bloomBuffer->unbind();
		sceneResult = _bloomBuffer->textureId();
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
		ScreenQuad::draw(_toneMapBuffer->textureId());
		framebuffer.unbind();
	} else {
		GLUtilities::blit(*_toneMapBuffer, framebuffer, 0, layer, Filter::LINEAR);
	}

	checkGLError();
}

void PostProcessStack::updateBlurPass(){
	const uint bw = _blurBuffer->width();
	const uint bh = _blurBuffer->height();
	_blurBuffer.reset(new GaussianBlur(bw, bh, _settings.bloomRadius, Layout::RGB16F));
}

void PostProcessStack::clean() {
	_blurBuffer->clean();
	_bloomBuffer->clean();
	_toneMapBuffer->clean();
}

void PostProcessStack::resize(unsigned int width, unsigned int height) {
	const glm::vec2 renderRes(width, height);
	_toneMapBuffer->resize(renderRes);
	_bloomBuffer->resize(renderRes);
	_blurBuffer->resize(width/2, height/2);
	checkGLError();
}

void PostProcessStack::interface(){
	ImGui::Checkbox("Bloom  ", &_settings.bloom);
	if(_settings.bloom) {
		ImGui::SameLine(120);
		ImGui::SliderFloat("Th.##Bloom", &_settings.bloomTh, 0.5f, 2.0f);

		ImGui::PushItemWidth(80);
		if(ImGui::InputInt("Rad.##Bloom", &_settings.bloomRadius, 1, 10)) {
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
