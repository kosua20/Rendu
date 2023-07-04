#include "PostProcessStack.hpp"
#include "scene/lights/Light.hpp"
#include "scene/Sky.hpp"
#include "system/System.hpp"
#include "graphics/GPU.hpp"


PostProcessStack::PostProcessStack(const glm::vec2 & resolution) : Renderer("Post process stack"), _bloomBuffer("Bloom"), _toneMapBuffer("Tonemap"), _dofDownscaledColor("DoF Downscale"), _dofCocAndDepth("DoF CoC"), _dofGatherBuffer("DoF gather"), _resultTexture("Postproc. result") {

	const uint renderWidth	= uint(resolution[0]);
	const uint renderHeight	= uint(resolution[1]);
	_bloomBuffer.setupAsDrawable(Layout::RGBA16F, renderWidth, renderHeight);
	_toneMapBuffer.setupAsDrawable(Layout::RGBA16F, renderWidth, renderHeight);
	_resultTexture.setupAsDrawable(Layout::RGBA16F, renderWidth, renderHeight);

	const uint halfRenderWidth = renderWidth/2;
	const uint halfRenderHeight = renderHeight/2;
	// Depth of field is performed at half resolution.
	_dofDownscaledColor.setupAsDrawable(Layout::RGBA16F, halfRenderWidth, halfRenderHeight);
	_dofCocAndDepth.setupAsDrawable(Layout::RG16F, halfRenderWidth, halfRenderHeight);
	_dofGatherBuffer.setupAsDrawable(Layout::RGBA16F, halfRenderWidth, halfRenderHeight);

	_blur		= std::unique_ptr<GaussianBlur>(new GaussianBlur(_settings.bloomRadius, 2, "Bloom"));
	_colorFormat = Layout::RGBA16F;
	
	_bloomProgram		= Resources::manager().getProgram2D("bloom");
	_bloomComposite 	= Resources::manager().getProgram2D("scale-texture");
	_toneMappingProgram	= Resources::manager().getProgram2D("tonemap");
	_fxaaProgram		= Resources::manager().getProgram2D("fxaa");

	_dofCocProgram = Resources::manager().getProgram2D("dof-coc");
	_dofGatherProgram = Resources::manager().getProgram2D("dof-gather");
	_dofCompositeProgram = Resources::manager().getProgram2D("dof-composite");
}

void PostProcessStack::process(const Texture& src, const glm::mat4 & proj, const Texture& depth, Texture& dst, uint layer) {

	const glm::vec2 invRenderSize = 1.0f / glm::vec2(dst.width, dst.height);


	GPUMarker marker("Post process");
	GPU::setDepthState(false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	if(_settings.dof){
		GPUMarker marker("Depth of field");
		// --- DoF pass ------
		// Compute circle of confidence along with the depth and downscaled color.
		GPU::bind(Load::Operation::DONTCARE, &_dofDownscaledColor, &_dofCocAndDepth);
		GPU::setViewport(_dofDownscaledColor);
		_dofCocProgram->use();
		_dofCocProgram->uniform("projParams", glm::vec2(proj[2][2], proj[3][2]));
		_dofCocProgram->uniform("focusDist", _settings.focusDist);
		_dofCocProgram->uniform("focusScale", _settings.focusScale);
		_dofCocProgram->texture(src, 0);
		_dofCocProgram->texture(depth, 1);
		GPU::drawQuad();
		// Gather from neighbor samples.
		GPU::bind(Load::Operation::DONTCARE, &_dofGatherBuffer);
		GPU::setViewport(_dofGatherBuffer);
		_dofGatherProgram->use();
		_dofGatherProgram->uniform("invSize", 1.0f/glm::vec2(_dofCocAndDepth.width, _dofCocAndDepth.height));
		_dofGatherProgram->texture(_dofDownscaledColor, 0);
		_dofGatherProgram->texture(_dofCocAndDepth, 1);
		GPU::drawQuad();
		// Finally composite back with full res image.
		GPU::bind(Load::Operation::DONTCARE, &_resultTexture);
		GPU::setViewport(_resultTexture);
		_dofCompositeProgram->use();
		_dofCompositeProgram->texture(src, 0);
		_dofCompositeProgram->texture(_dofGatherBuffer, 1);
		GPU::drawQuad();
	} else {
		// Else just copy the input texture to our internal result.
		GPU::bind(Load::Operation::DONTCARE, &_resultTexture);
		GPU::setViewport(_resultTexture);
		Resources::manager().getProgram2D("passthrough-pixelperfect")->use();
		Resources::manager().getProgram2D("passthrough-pixelperfect")->texture(src, 0);
		GPU::drawQuad();
	}

	if(_settings.bloom) {
		GPUMarker marker("Bloom");

		// --- Bloom selection pass ------
		{
			GPUMarker marker("Extraction");
			GPU::bind(Load::Operation::DONTCARE, &_bloomBuffer);
			GPU::setViewport(_bloomBuffer);
			_bloomProgram->use();
			_bloomProgram->uniform("luminanceTh", _settings.bloomTh);
			_bloomProgram->texture(_resultTexture, 0);
			GPU::drawQuad();
		}
		
		// --- Bloom blur pass ------
		_blur->process(_bloomBuffer, _bloomBuffer);
		
		// Add back the scene content.
		{
			GPUMarker marker("Compositing");
			GPU::bind(Load::Operation::LOAD, &_resultTexture);
			GPU::setViewport(_resultTexture);
			GPU::setBlendState(true, BlendEquation::ADD, BlendFunction::ONE, BlendFunction::ONE);
			_bloomComposite->use();
			_bloomComposite->uniform("scale", _settings.bloomMix);
			_bloomComposite->texture(_bloomBuffer, 0);
			GPU::drawQuad();
			GPU::setBlendState(false);
		}
		// Steps below ensures that we will always have an intermediate target.
	}

	// --- Tonemapping pass ------
	{
		GPUMarker marker("Tonemap");
		GPU::bind(Load::Operation::DONTCARE, &_toneMapBuffer);
		GPU::setViewport(_toneMapBuffer);
		_toneMappingProgram->use();
		_toneMappingProgram->uniform("customExposure", _settings.exposure);
		_toneMappingProgram->uniform("apply", _settings.tonemap);
		_toneMappingProgram->texture(_resultTexture, 0);
		GPU::drawQuad();
	}

	if(_settings.fxaa) {
		GPUMarker marker("FXAA");
		GPU::bind(layer, 0, Load::Operation::LOAD, &dst);
		GPU::setViewport(dst);
		_fxaaProgram->use();
		_fxaaProgram->uniform("inverseScreenSize", invRenderSize);
		_fxaaProgram->texture(_toneMapBuffer, 0);
		GPU::drawQuad();
	} else {
		GPU::blit(_toneMapBuffer, dst, 0, layer, Filter::LINEAR);
	}

}

void PostProcessStack::updateBlurPass(){
	_blur.reset(new GaussianBlur(_settings.bloomRadius, 2, "Bloom"));
}

void PostProcessStack::resize(uint width, uint height) {
	const glm::ivec2 renderRes(width, height);
	_toneMapBuffer.resize(renderRes);
	_bloomBuffer.resize(renderRes);
	_resultTexture.resize(renderRes);
	_dofGatherBuffer.resize(renderRes/2);
	_dofDownscaledColor.resize(renderRes/2);
	_dofCocAndDepth.resize(renderRes/2);
}

void PostProcessStack::interface(){
	ImGui::Checkbox("DoF", &_settings.dof); ImGui::SameLine();
	ImGui::Checkbox("Bloom", &_settings.bloom);ImGui::SameLine();
	ImGui::Checkbox("Tonemap", &_settings.tonemap);ImGui::SameLine();
	ImGui::Checkbox("FXAA", &_settings.fxaa);

	if(_settings.dof) {
		ImGui::Text("DoF  "); ImGui::SameLine();
		ImGui::PushItemWidth(80);
		ImGui::SliderFloat("Dist.##DoF", &_settings.focusDist, 0.1f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
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
