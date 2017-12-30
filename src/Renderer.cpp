#include <stdio.h>
#include <iostream>
#include <vector>

#include "Object.hpp"
#include "lights/DirectionalLight.hpp"
#include "lights/PointLight.hpp"
#include "input/Input.hpp"
#include "Renderer.hpp"


Renderer::~Renderer(){}

Renderer::Renderer(Config & config, std::shared_ptr<Scene> & scene) : _config(config) {
	
	// Initial render resolution.
	_renderResolution = (_config.internalVerticalResolution/_config.screenResolution[1]) * _config.screenResolution;
	// Setup camera parameters.
	_camera.projection(config.screenResolution[0]/config.screenResolution[1], 1.3f, 0.01f, 200.0f);
	
	const int renderWidth = (int)_renderResolution[0];
	const int renderHeight = (int)_renderResolution[1];
	const int renderHalfWidth = (int)(0.5f * _renderResolution[0]);
	const int renderHalfHeight = (int)(0.5f * _renderResolution[1]);
	// Find the closest power of 2 size.
	const int renderPow2Size = (int)std::pow(2,(int)floor(log2(_renderResolution[0])));
	_gbuffer = std::make_shared<Gbuffer>(renderWidth, renderHeight);
	_ssaoFramebuffer = std::make_shared<Framebuffer>(renderHalfWidth, renderHalfHeight, GL_RED, GL_UNSIGNED_BYTE, GL_RED, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
	_ssaoBlurFramebuffer = std::make_shared<Framebuffer>(renderWidth, renderHeight, GL_RED, GL_UNSIGNED_BYTE, GL_RED, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
	_sceneFramebuffer = std::make_shared<Framebuffer>(renderWidth, renderHeight, GL_RGBA, GL_FLOAT, GL_RGBA16F, GL_LINEAR,GL_CLAMP_TO_EDGE, false);
	_bloomFramebuffer = std::make_shared<Framebuffer>(renderPow2Size, renderPow2Size, GL_RGB, GL_FLOAT, GL_RGB16F, GL_LINEAR,GL_CLAMP_TO_EDGE, false);
	_toneMappingFramebuffer = std::make_shared<Framebuffer>(renderWidth, renderHeight, GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA, GL_LINEAR,GL_CLAMP_TO_EDGE, false);
	_fxaaFramebuffer = std::make_shared<Framebuffer>(renderWidth, renderHeight, GL_RGBA,GL_UNSIGNED_BYTE, GL_RGBA, GL_LINEAR,GL_CLAMP_TO_EDGE, false);
	
	_blurBuffer = std::make_shared<Blur>(renderPow2Size, renderPow2Size, 2);
	
	PointLight::loadProgramAndGeometry();
	
	checkGLError();

	// GL options
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glBlendEquation (GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	checkGLError();

	_scene = scene;
	_scene->init();

	std::map<std::string, GLuint> ambientTextures = _gbuffer->textureIds({ TextureType::Albedo, TextureType::Normal, TextureType::Depth, TextureType::Effects });
	ambientTextures["ssaoTexture"] = _ssaoBlurFramebuffer->textureId();
	_ambientScreen.init(ambientTextures, _scene->backgroundReflection, _scene->backgroundIrradiance);
	
	const std::vector<TextureType> includedTextures = { TextureType::Albedo, TextureType::Depth, TextureType::Normal, TextureType::Effects };
	
	for(auto& dirLight : _scene->directionalLights){
		dirLight.init(_gbuffer->textureIds(includedTextures));
	}
	for(auto& pointLight : _scene->pointLights){
		pointLight.init(_gbuffer->textureIds(includedTextures));
	}
	
	_ssaoBlurScreen.init(_ssaoFramebuffer->textureId(), "boxblur_float");
	_bloomScreen.init(_sceneFramebuffer->textureId(), "bloom");
	_toneMappingScreen.init(_sceneFramebuffer->textureId(), "tonemap");
	_fxaaScreen.init(_toneMappingFramebuffer->textureId(), "fxaa");
	_finalScreen.init(_fxaaFramebuffer->textureId(), "final_screenquad");
	checkGLError();
	
}


void Renderer::draw() {

	glm::vec2 invRenderSize = 1.0f / _renderResolution;
	
	// --- Light pass -------
	
	// Draw the scene inside the framebuffer.
	for(auto& dirLight : _scene->directionalLights){

		dirLight.bind();
		for(auto& object : _scene->objects){
			object.drawDepth(dirLight.mvp());
		}
		dirLight.blurAndUnbind();
	}
	
	// ----------------------
	
	// --- Scene pass -------
	// Bind the full scene framebuffer.
	_gbuffer->bind();
	// Set screen viewport
	glViewport(0,0,_gbuffer->width(),_gbuffer->height());
	
	// Clear the depth buffer (we know we will draw everywhere, no need to clear color.
	glClear(GL_DEPTH_BUFFER_BIT);
	
	for(auto & object : _scene->objects){
		object.draw(_camera.view(), _camera.projection());
	}
	
	for(auto& pointLight : _scene->pointLights){
		pointLight.drawDebug(_camera.view(), _camera.projection());
	}
	
	// No need to write the skybox depth to the framebuffer.
	glDepthMask(GL_FALSE);
	// Accept a depth of 1.0 (far plane).
	glDepthFunc(GL_LEQUAL);
	// draw background.
	_scene->background.draw(_camera.view(), _camera.projection());
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	
	
	// Unbind the full scene framebuffer.
	_gbuffer->unbind();
	// ----------------------
	
	glDisable(GL_DEPTH_TEST);
	
	// --- SSAO pass
	_ssaoFramebuffer->bind();
	glViewport(0,0,_ssaoFramebuffer->width(), _ssaoFramebuffer->height());
	_ambientScreen.drawSSAO(_camera.view(), _camera.projection());
	_ssaoFramebuffer->unbind();
	
	// --- SSAO blurring pass
	_ssaoBlurFramebuffer->bind();
	glViewport(0,0,_ssaoBlurFramebuffer->width(), _ssaoBlurFramebuffer->height());
	_ssaoBlurScreen.draw();
	_ssaoBlurFramebuffer->unbind();
	
	// --- Gbuffer composition pass
	_sceneFramebuffer->bind();
	
	glViewport(0,0,_sceneFramebuffer->width(), _sceneFramebuffer->height());
	
	_ambientScreen.draw(_camera.view(), _camera.projection());
	
	glEnable(GL_BLEND);
	for(auto& dirLight : _scene->directionalLights){
		dirLight.draw(_camera.view(), _camera.projection());
	}
	glCullFace(GL_FRONT);
	for(auto& pointLight : _scene->pointLights){
		pointLight.draw(_camera.view(), _camera.projection(), invRenderSize);
	}
	
	glDisable(GL_BLEND);
	glCullFace(GL_BACK);
	_sceneFramebuffer->unbind();
	
	// --- Bloom selection pass ------
	_bloomFramebuffer->bind();
	glViewport(0,0,_bloomFramebuffer->width(), _bloomFramebuffer->height());
	_bloomScreen.draw();
	_bloomFramebuffer->unbind();
	
	// --- Bloom blur pass ------
	_blurBuffer->process(_bloomFramebuffer->textureId());
	
	// Draw the blurred bloom back into the scene framebuffer.
	_sceneFramebuffer->bind();
	glViewport(0,0,_sceneFramebuffer->width(), _sceneFramebuffer->height());
	glEnable(GL_BLEND);
	_blurBuffer->draw();
	glDisable(GL_BLEND);
	_sceneFramebuffer->unbind();
	
	
	// --- Tonemapping pass ------
	_toneMappingFramebuffer->bind();
	glViewport(0,0,_toneMappingFramebuffer->width(), _toneMappingFramebuffer->height());
	_toneMappingScreen.draw();
	_toneMappingFramebuffer->unbind();
	
	// --- FXAA pass -------
	// Bind the post-processing framebuffer.
	_fxaaFramebuffer->bind();
	glViewport(0,0,_fxaaFramebuffer->width(), _fxaaFramebuffer->height());
	_fxaaScreen.draw( invRenderSize );
	_fxaaFramebuffer->unbind();
	
	// --- Final pass -------
	// We now render a full screen quad in the default framebuffer, using sRGB space.
	glEnable(GL_FRAMEBUFFER_SRGB);
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	_finalScreen.draw();
	glDisable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST);
	
}

void Renderer::update(){
	if(Input::manager().resized()){
		resize(Input::manager().size()[0], Input::manager().size()[1]);
	}
}

void Renderer::physics(double fullTime, double frameTime){
	_camera.update(frameTime);
	_scene->update(fullTime, frameTime);
}


void Renderer::clean() const {
	// Clean objects.
	_scene->clean();
	_ambientScreen.clean();
	_fxaaScreen.clean();
	_ssaoBlurScreen.clean();
	_bloomScreen.clean();
	_toneMappingScreen.clean();
	_finalScreen.clean();
	_gbuffer->clean();
	_blurBuffer->clean();
	_ssaoFramebuffer->clean();
	_ssaoBlurFramebuffer->clean();
	_bloomFramebuffer->clean();
	_sceneFramebuffer->clean();
	_toneMappingFramebuffer->clean();
	_fxaaFramebuffer->clean();
}


void Renderer::resize(int width, int height){
	_config.screenResolution[0] = float(width > 0 ? width : 1);
	_config.screenResolution[1] = float(height > 0 ? height : 1);
	// Same aspect ratio as the display resolution
	_renderResolution = (_config.internalVerticalResolution/_config.screenResolution[1]) * _config.screenResolution;
	
	//Update the size of the viewport.
	// TODO: remove call below probably.
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	// Update the projection aspect ratio.
	_camera.ratio(_renderResolution[0] / _renderResolution[1]);
	// Resize the framebuffers.
	_gbuffer->resize(_renderResolution);
	_ssaoFramebuffer->resize(0.5f * _renderResolution);
	_ssaoBlurFramebuffer->resize(_renderResolution);
	_sceneFramebuffer->resize(_renderResolution);
	_toneMappingFramebuffer->resize(_renderResolution);
	_fxaaFramebuffer->resize(_renderResolution);
}



