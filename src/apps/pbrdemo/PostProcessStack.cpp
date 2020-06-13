#include "PostProcessStack.hpp"
#include "scene/lights/Light.hpp"
#include "scene/Sky.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/ScreenQuad.hpp"

PostProcessStack::PostProcessStack(const glm::vec2 & resolution){
	const int renderWidth	= int(resolution[0]);
	const int renderHeight	= int(resolution[1]);
	const Descriptor desc = {Layout::RGB16F, Filter::LINEAR_NEAREST, Wrap::CLAMP};
	_bloomBuffer	= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, desc, false));
	_toneMapBuffer 	= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, desc, false));
	_resultFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, desc, false));

	// Depth of field is performed at half resolution.
	const Descriptor dofCocDesc = {Layout::RG16F, Filter::NEAREST, Wrap::CLAMP};
	const Descriptor dofGatherDesc = {Layout::RGBA16F, Filter::LINEAR_NEAREST, Wrap::CLAMP};
	_dofCocBuffer 	= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth/2, renderHeight/2, {desc, dofCocDesc}, false));
	_dofGatherBuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth/2, renderHeight/2, dofGatherDesc, false));

	_blur		= std::unique_ptr<GaussianBlur>(new GaussianBlur(_settings.bloomRadius, 2));
	_preferredFormat.push_back(desc);
	_needsDepth = false;
	_bloomProgram		= Resources::manager().getProgram2D("bloom");
	_bloomComposite 	= Resources::manager().getProgram2D("scale-texture");
	_toneMappingProgram	= Resources::manager().getProgram2D("tonemap");
	_fxaaProgram		= Resources::manager().getProgram2D("fxaa");

	_dofCocProgram = Resources::manager().getProgram2D("dof-coc");
	_dofGatherProgram = Resources::manager().getProgram2D("dof-gather");
	_dofCompositeProgram = Resources::manager().getProgram2D("dof-composite");
	checkGLError();
}

void PostProcessStack::process(const Texture * texture, const glm::mat4 & proj, const Texture * depth, Framebuffer & framebuffer, size_t layer) {

	const glm::vec2 invRenderSize = 1.0f / glm::vec2(framebuffer.width(), framebuffer.height());

	GLUtilities::setDepthState(false);
	GLUtilities::setBlendState(false);

	if(_settings.dof){
		// --- DoF pass ------
		// Compute circle of confidence along with the depth and downscaled color.
		_dofCocBuffer->bind();
		_dofCocBuffer->setViewport();
		_dofCocProgram->use();
		_dofCocProgram->uniform("projParams", glm::vec2(proj[2][2], proj[3][2]));
		_dofCocProgram->uniform("focusDist", _settings.focusDist);
		_dofCocProgram->uniform("focusScale", _settings.focusScale);
		GLUtilities::bindTexture(texture, 0);
		GLUtilities::bindTexture(depth, 1);
		ScreenQuad::draw();
		// Gather from neighbor samples.
		_dofGatherBuffer->bind();
		_dofGatherBuffer->setViewport();
		_dofGatherProgram->use();
		_dofGatherProgram->uniform("invSize", 1.0f/glm::vec2(_dofCocBuffer->width(), _dofCocBuffer->height()));
		GLUtilities::bindTexture(_dofCocBuffer->texture(0), 0);
		GLUtilities::bindTexture(_dofCocBuffer->texture(1), 1);
		ScreenQuad::draw();
		// Finally composite back with full res image.
		_resultFramebuffer->bind();
		_resultFramebuffer->setViewport();
		_dofCompositeProgram->use();
		GLUtilities::bindTexture(texture, 0);
		GLUtilities::bindTexture(_dofGatherBuffer->texture(), 1);
		ScreenQuad::draw();
		_resultFramebuffer->unbind();
	} else {
		// Else just copy the input texture to our internal result.
		_resultFramebuffer->bind();
		_resultFramebuffer->setViewport();
		Resources::manager().getProgram2D("passthrough-pixelperfect")->use();
		ScreenQuad::draw(texture);
		_resultFramebuffer->unbind();
	}

	if(_settings.bloom) {
		// --- Bloom selection pass ------
		_bloomBuffer->bind();
		_bloomBuffer->setViewport();
		_bloomProgram->use();
		_bloomProgram->uniform("luminanceTh", _settings.bloomTh);
		ScreenQuad::draw(_resultFramebuffer->texture());
		_bloomBuffer->unbind();
		
		// --- Bloom blur pass ------
		_blur->process(_bloomBuffer->texture(), *_bloomBuffer);
		
		// Add back the scene content.
		_resultFramebuffer->bind();
		_resultFramebuffer->setViewport();
		GLUtilities::setBlendState(true, BlendEquation::ADD, BlendFunction::ONE, BlendFunction::ONE);
		_bloomComposite->use();
		_bloomComposite->uniform("scale", _settings.bloomMix);
		ScreenQuad::draw(_bloomBuffer->texture());
		GLUtilities::setBlendState(false);
		_resultFramebuffer->unbind();
		// Steps below ensures that we will always have an intermediate target.
	}

	// --- Tonemapping pass ------
	_toneMapBuffer->bind();
	_toneMapBuffer->setViewport();
	_toneMappingProgram->use();
	_toneMappingProgram->uniform("customExposure", _settings.exposure);
	_toneMappingProgram->uniform("apply", _settings.tonemap);
	ScreenQuad::draw(_resultFramebuffer->texture());
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

void PostProcessStack::resize(unsigned int width, unsigned int height) {
	const glm::ivec2 renderRes(width, height);
	_toneMapBuffer->resize(renderRes);
	_bloomBuffer->resize(renderRes);
	_resultFramebuffer->resize(renderRes);
	_dofGatherBuffer->resize(renderRes/4);
	_dofCocBuffer->resize(renderRes/4);
	checkGLError();
}

void PostProcessStack::interface(){
	ImGui::Checkbox("DoF", &_settings.dof); ImGui::SameLine();
	ImGui::Checkbox("Bloom", &_settings.bloom);ImGui::SameLine();
	ImGui::Checkbox("Tonemap", &_settings.tonemap);ImGui::SameLine();
	ImGui::Checkbox("FXAA", &_settings.fxaa);

	if(_settings.dof) {
		ImGui::Text("DoF  "); ImGui::SameLine();
		ImGui::PushItemWidth(80);
		ImGui::SliderFloat("Dist.##DoF", &_settings.focusDist, 0.1f, 10.0f, "%.3f", 2.0f);
		ImGui::SameLine();
		ImGui::SliderFloat("Str.##DoF", &_settings.focusScale, 1.0f, 30.0f);
		ImGui::PopItemWidth();
	}

	if(_settings.bloom) {
		ImGui::Text("Bloom"); ImGui::SameLine();
		ImGui::PushItemWidth(80);
		ImGui::SliderFloat("Th.##Bloom", &_settings.bloomTh, 0.5f, 2.0f);
		ImGui::SameLine();
		ImGui::SliderFloat("Mix##Bloom", &_settings.bloomMix, 0.0f, 1.5f);
		if(ImGui::InputInt("Rad.##Bloom", &_settings.bloomRadius, 1, 10)) {
			_settings.bloomRadius = std::max(1, _settings.bloomRadius);
			updateBlurPass();
		}
		ImGui::PopItemWidth();
	}

	if(_settings.tonemap) {
		ImGui::PushItemWidth(160);
		ImGui::SliderFloat("Exposure", &_settings.exposure, 0.1f, 10.0f);
		ImGui::PopItemWidth();
	}
}
