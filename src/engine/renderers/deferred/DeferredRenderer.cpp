#include "DeferredRenderer.hpp"
#include "../../input/Input.hpp"
#include "../../lights/DirectionalLight.hpp"
#include "../../lights/PointLight.hpp"
#include "../../lights/SpotLight.hpp"
#include "../../helpers/InterfaceUtilities.hpp"


DeferredRenderer::DeferredRenderer(Config & config) : Renderer(config) {
	
	// Setup camera parameters.
	_userCamera.projection(config.screenResolution[0]/config.screenResolution[1], 1.3f, 0.01f, 200.0f);
	
	const int renderWidth = (int)_renderResolution[0];
	const int renderHeight = (int)_renderResolution[1];
	const int renderHalfWidth = (int)(0.5f * _renderResolution[0]);
	const int renderHalfHeight = (int)(0.5f * _renderResolution[1]);
	// Find the closest power of 2 size.
	const int renderPow2Size = (int)std::pow(2,(int)floor(log2(_renderResolution[0])));
	_gbuffer = std::make_shared<Gbuffer>(renderWidth, renderHeight);
	_ssaoFramebuffer = std::make_shared<Framebuffer>(renderHalfWidth, renderHalfHeight, GL_RED, GL_UNSIGNED_BYTE, GL_RED, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
	_sceneFramebuffer = std::make_shared<Framebuffer>(renderWidth, renderHeight, GL_RGBA, GL_FLOAT, GL_RGBA16F, GL_LINEAR,GL_CLAMP_TO_EDGE, false);
	_bloomFramebuffer = std::make_shared<Framebuffer>(renderPow2Size, renderPow2Size, GL_RGB, GL_FLOAT, GL_RGB16F, GL_LINEAR,GL_CLAMP_TO_EDGE, false);
	_toneMappingFramebuffer = std::make_shared<Framebuffer>(renderWidth, renderHeight, GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA, GL_LINEAR,GL_CLAMP_TO_EDGE, false);
	_fxaaFramebuffer = std::make_shared<Framebuffer>(renderWidth, renderHeight, GL_RGBA,GL_UNSIGNED_BYTE, GL_RGBA, GL_LINEAR,GL_CLAMP_TO_EDGE, false);
	
	_blurBuffer = std::make_shared<GaussianBlur>(renderPow2Size, renderPow2Size, 2, GL_RGB, GL_FLOAT, GL_RGB16F);
	_blurSSAOBuffer = std::make_shared<BoxBlur>(renderHalfWidth, renderHalfHeight, true, GL_RED, GL_UNSIGNED_BYTE, GL_RED, GL_CLAMP_TO_EDGE);
	
	checkGLError();

	// GL options
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glBlendEquation (GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	
	_bloomProgram = Resources::manager().getProgram2D("bloom");
	_toneMappingProgram = Resources::manager().getProgram2D("tonemap");
	_fxaaProgram = Resources::manager().getProgram2D("fxaa");
	_finalProgram = Resources::manager().getProgram2D("final_screenquad");
	
	std::vector<GLuint> ambientTextures = _gbuffer->textureIds({ TextureType::Albedo, TextureType::Normal, TextureType::Depth, TextureType::Effects });
	// Add the SSAO result.
	ambientTextures.push_back(_blurSSAOBuffer->textureId());
	_ambientScreen.init(ambientTextures);
	
	checkGLError();
	
}

void DeferredRenderer::setScene(std::shared_ptr<Scene> scene){
	_scene = scene;
	if(!scene){
		return;
	}
	_scene->init();
	
	_ambientScreen.setSceneParameters(_scene->backgroundReflection, _scene->backgroundIrradiance);
	
	const std::vector<TextureType> includedTextures = { TextureType::Albedo, TextureType::Normal, TextureType::Depth, TextureType::Effects };
	for(auto& dirLight : _scene->directionalLights){
		dirLight.init(_gbuffer->textureIds(includedTextures));
	}
	for(auto& pointLight : _scene->pointLights){
		pointLight.init(_gbuffer->textureIds(includedTextures));
	}
	for(auto& spotLight : _scene->spotLights){
		spotLight.init(_gbuffer->textureIds(includedTextures));
	}
	checkGLError();
}

void DeferredRenderer::draw() {
	
	if(!_scene){
		glClearColor(0.2f,0.2,0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}
	
	// Interface.
	/// \todo Move to a separate function maybe?
	if(ImGui::Begin("Renderer")){
		ImGui::Checkbox("Show debug", &_debugVisualization);
	}
	ImGui::End();
	
	glm::vec2 invRenderSize = 1.0f / _renderResolution;
	
	// --- Light pass -------
	
	// Draw the scene inside the framebuffer.
	for(auto& dirLight : _scene->directionalLights){
		dirLight.drawShadow(_scene->objects);
	}
	for(auto& shadowLight : _scene->spotLights){
		shadowLight.drawShadow(_scene->objects);
	}
	for(auto& pointLight : _scene->pointLights){
		pointLight.drawShadow(_scene->objects);
	}
	// ----------------------
	
	// --- Scene pass -------
	// Bind the full scene framebuffer.
	_gbuffer->bind();
	// Set screen viewport
	glViewport(0,0, (GLsizei)_gbuffer->width(), (GLsizei)_gbuffer->height());
	
	// Clear the depth buffer (we know we will draw everywhere, no need to clear color.
	glClear(GL_DEPTH_BUFFER_BIT);
	
	for(auto & object : _scene->objects){
		object.draw(_userCamera.view(), _userCamera.projection());
	}
	
	if(_debugVisualization){
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_CULL_FACE);
		for(auto& pointLight : _scene->pointLights){
			pointLight.drawDebug(_userCamera.view(), _userCamera.projection());
		}
		for(auto& dirLight : _scene->directionalLights){
			dirLight.drawDebug(_userCamera.view(), _userCamera.projection());
		}
		for(auto& spotLight : _scene->spotLights){
			spotLight.drawDebug(_userCamera.view(), _userCamera.projection());
		}
		glEnable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	// No need to write the skybox depth to the framebuffer.
	glDepthMask(GL_FALSE);
	// Accept a depth of 1.0 (far plane).
	glDepthFunc(GL_LEQUAL);
	// draw background.
	_scene->background.draw(_userCamera.view(), _userCamera.projection());
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	
	
	// Unbind the full scene framebuffer.
	_gbuffer->unbind();
	// ----------------------
	
	glDisable(GL_DEPTH_TEST);
	
	// --- SSAO pass
	_ssaoFramebuffer->bind();
	_ssaoFramebuffer->setViewport();
	_ambientScreen.drawSSAO(_userCamera.projection());
	_ssaoFramebuffer->unbind();
	
	// --- SSAO blurring pass
	_blurSSAOBuffer->process(_ssaoFramebuffer->textureId());
	
	// --- Gbuffer composition pass
	_sceneFramebuffer->bind();
	_sceneFramebuffer->setViewport();
	
	_ambientScreen.draw(_userCamera.view(), _userCamera.projection());
	
	glEnable(GL_BLEND);
	for(auto& dirLight : _scene->directionalLights){
		dirLight.draw(_userCamera.view(), _userCamera.projection());
	}
	glCullFace(GL_FRONT);
	for(auto& pointLight : _scene->pointLights){
		pointLight.draw(_userCamera.view(), _userCamera.projection(), invRenderSize);
	}
	for(auto& spotLight : _scene->spotLights){
		spotLight.draw(_userCamera.view(), _userCamera.projection(), invRenderSize);
	}
	glCullFace(GL_BACK);
	glDisable(GL_BLEND);
	
	_sceneFramebuffer->unbind();
	
	// --- Bloom selection pass ------
	_bloomFramebuffer->bind();
	_bloomFramebuffer->setViewport();
	glUseProgram(_bloomProgram->id());
	ScreenQuad::draw(_sceneFramebuffer->textureId());
	_bloomFramebuffer->unbind();
	
	// --- Bloom blur pass ------
	_blurBuffer->process(_bloomFramebuffer->textureId());
	
	// Draw the blurred bloom back into the scene framebuffer.
	_sceneFramebuffer->bind();
	_sceneFramebuffer->setViewport();
	glEnable(GL_BLEND);
	_blurBuffer->draw();
	glDisable(GL_BLEND);
	_sceneFramebuffer->unbind();
	
	
	// --- Tonemapping pass ------
	_toneMappingFramebuffer->bind();
	_toneMappingFramebuffer->setViewport();
	glUseProgram(_toneMappingProgram->id());
	ScreenQuad::draw(_sceneFramebuffer->textureId());
	_toneMappingFramebuffer->unbind();
	
	// --- FXAA pass -------
	// Bind the post-processing framebuffer.
	_fxaaFramebuffer->bind();
	_fxaaFramebuffer->setViewport();
	glUseProgram(_fxaaProgram->id());
	glUniform2fv(_fxaaProgram->uniform("inverseScreenSize"), 1, &(invRenderSize[0]));
	ScreenQuad::draw(_toneMappingFramebuffer->textureId());
	_fxaaFramebuffer->unbind();
	
	// --- Final pass -------
	// We now render a full screen quad in the default framebuffer, using sRGB space.
	glEnable(GL_FRAMEBUFFER_SRGB);
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	glUseProgram(_finalProgram->id());
	ScreenQuad::draw(_fxaaFramebuffer->textureId());
	glDisable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST);
	
	checkGLError();
}

void DeferredRenderer::update(){
	Renderer::update();
	_userCamera.update();
	
	if(Input::manager().triggered(Input::KeyO)){
		GLUtilities::saveDefaultFramebuffer((unsigned int)_config.screenResolution[0], (unsigned int)_config.screenResolution[1], "./test-default");
	}
}

void DeferredRenderer::physics(double fullTime, double frameTime){
	_userCamera.physics(frameTime);
	if(_scene){
		_scene->update(fullTime, frameTime);
	}
}


void DeferredRenderer::clean() const {
	Renderer::clean();
	// Clean objects.
	_ambientScreen.clean();
	_gbuffer->clean();
	_blurBuffer->clean();
	_blurSSAOBuffer->clean();
	_ssaoFramebuffer->clean();
	_bloomFramebuffer->clean();
	_sceneFramebuffer->clean();
	_toneMappingFramebuffer->clean();
	_fxaaFramebuffer->clean();
	if(_scene){
		_scene->clean();
	}
}


void DeferredRenderer::resize(unsigned int width, unsigned int height){
	Renderer::updateResolution(width, height);
	// Resize the framebuffers.
	_gbuffer->resize(_renderResolution);
	_ssaoFramebuffer->resize(0.5f * _renderResolution);
	_blurSSAOBuffer->resize(_ssaoFramebuffer->width(), _ssaoFramebuffer->height());
	_toneMappingFramebuffer->resize(_renderResolution);
	_fxaaFramebuffer->resize(_renderResolution);
	if(_scene){
		_sceneFramebuffer->resize(_renderResolution);
	}
	checkGLError();
}



