#include "PostProcessStack.hpp"
#include "scene/lights/Light.hpp"
#include "scene/Sky.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"
#include <chrono>

PostProcessStack::PostProcessStack(const glm::vec2 & resolution){

	_renderResolution = resolution;
	const int renderWidth	= int(_renderResolution[0]);
	const int renderHeight	= int(_renderResolution[1]);
	const int renderHWidth  = int(0.5f * _renderResolution[0]);
	const int renderHHeight = int(0.5f * _renderResolution[1]);

	_bloomBuffer	= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, Layout::RGB16F, false));
	_toneMapBuffer 	= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, Layout::RGB16F, false));
	_fxaaBuffer		= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, Layout::RGB16F, false));
	_blurBuffer		= std::unique_ptr<GaussianBlur>(new GaussianBlur(renderHWidth, renderHHeight, _settings.bloomRadius, Layout::RGB16F));

	_bloomProgram		   = Resources::manager().getProgram2D("bloom");
	_bloomCompositeProgram = Resources::manager().getProgram2D("bloom-composite");
	_toneMappingProgram	   = Resources::manager().getProgram2D("tonemap");
	_fxaaProgram		   = Resources::manager().getProgram2D("fxaa");
	checkGLError();
}

void PostProcessStack::process(const Texture * texture) {

	const glm::vec2 invRenderSize = 1.0f / _renderResolution;

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
	_renderResult = _toneMapBuffer->textureId();
	
	if(_settings.fxaa) {
		_fxaaBuffer->bind();
		_fxaaBuffer->setViewport();
		_fxaaProgram->use();
		_fxaaProgram->uniform("inverseScreenSize", invRenderSize);
		ScreenQuad::draw(_renderResult);
		_fxaaBuffer->unbind();
		_renderResult = _fxaaBuffer->textureId();
	}

	checkGLError();
}

void PostProcessStack::updateBlurPass(){
	_blurBuffer.reset(new GaussianBlur(int(_renderResolution[0] / 2), int(_renderResolution[1] / 2), _settings.bloomRadius, Layout::RGB16F));
}

void PostProcessStack::clean() {
	_blurBuffer->clean();
	_bloomBuffer->clean();
	_toneMapBuffer->clean();
	_fxaaBuffer->clean();
}

void PostProcessStack::resize(unsigned int width, unsigned int height) {
	_renderResolution[0] = float(width);
	_renderResolution[1] = float(height);
	const unsigned int hWidth  = uint(_renderResolution[0] / 2.0f);
	const unsigned int hHeight = uint(_renderResolution[1] / 2.0f);
	// Resize the framebuffers.
	_toneMapBuffer->resize(_renderResolution);
	_fxaaBuffer->resize(_renderResolution);
	_bloomBuffer->resize(_renderResolution);
	_blurBuffer->resize(hWidth, hHeight);
	checkGLError();
}
