#include "DeferredRenderer.hpp"
#include "input/Input.hpp"
#include "scene/lights/DirectionalLight.hpp"
#include "scene/lights/PointLight.hpp"
#include "scene/lights/SpotLight.hpp"
#include "scene/Sky.hpp"
#include "helpers/System.hpp"
#include <chrono>

DeferredRenderer::DeferredRenderer(RenderingConfig & config) : Renderer(config) {
	
	// Setup camera parameters.
	_userCamera.projection(config.screenResolution[0]/config.screenResolution[1], 1.3f, 0.01f, 200.0f);
	_cameraFOV = _userCamera.fov() * 180.0f / float(M_PI);
	
	const int renderWidth = (int)_renderResolution[0];
	const int renderHeight = (int)_renderResolution[1];
	const int renderHalfWidth = (int)(0.5f * _renderResolution[0]);
	const int renderHalfHeight = (int)(0.5f * _renderResolution[1]);
	
	// G-buffer setup.
	const Descriptor albedoDesc = { GL_RGBA16F, GL_NEAREST_MIPMAP_NEAREST, GL_CLAMP_TO_EDGE };
	const Descriptor normalDesc = { GL_RGB32F, GL_NEAREST_MIPMAP_NEAREST, GL_CLAMP_TO_EDGE };
	const Descriptor effectsDesc = { GL_RGB8, GL_NEAREST_MIPMAP_NEAREST, GL_CLAMP_TO_EDGE };
	const Descriptor depthDesc = { GL_DEPTH_COMPONENT32F, GL_NEAREST_MIPMAP_NEAREST, GL_CLAMP_TO_EDGE };
	const std::vector<Descriptor> descs = {albedoDesc, normalDesc, effectsDesc, depthDesc};
	_gbuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, descs, false));
	
	// Other framebuffers.
	_ssaoPass = std::unique_ptr<SSAO>(new SSAO(renderHalfWidth, renderHalfHeight, 0.5f));
	_sceneFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, GL_RGBA16F, false));
	_bloomFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, GL_RGB16F, false));
	_toneMappingFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, GL_RGB16F, false));
	_fxaaFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, GL_RGB16F, false));
	_blurBuffer = std::unique_ptr<GaussianBlur>(new GaussianBlur(renderHalfWidth, renderHalfHeight, 4, GL_RGB16F));
	
	checkGLError();

	// GL options
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glBlendEquation (GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	
	_bloomProgram = Resources::manager().getProgram2D("bloom");
	_bloomCompositeProgram = Resources::manager().getProgram2D("bloom-composite");
	_toneMappingProgram = Resources::manager().getProgram2D("tonemap");
	_fxaaProgram = Resources::manager().getProgram2D("fxaa");
	_finalProgram = Resources::manager().getProgram2D("final_screenquad");
	
	_skyboxProgram = Resources::manager().getProgram("skybox_gbuffer");
	_bgProgram = Resources::manager().getProgram("background_gbuffer");
	_atmoProgram = Resources::manager().getProgram("atmosphere_gbuffer", "background_gbuffer", "atmosphere_gbuffer");;
	_parallaxProgram = Resources::manager().getProgram("parallax_gbuffer");
	_objectProgram = Resources::manager().getProgram("object_gbuffer");
	_objectNoUVsProgram = Resources::manager().getProgram("object_no_uv_gbuffer");
	
	const std::vector<GLuint> ambientTextures = _gbuffer->textureIds();
	
	// Add the SSAO result.
	_ambientScreen.init(ambientTextures[0], ambientTextures[1], ambientTextures[2], _gbuffer->depthId(), _ssaoPass->textureId());
	
	checkGLError();
	
}

void DeferredRenderer::setScene(std::shared_ptr<Scene> scene){
	_scene = scene;
	if(!scene){
		return;
	}
	
	auto start = std::chrono::steady_clock::now();
	_scene->init(Storage::GPU);
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
	Log::Info() << "Loading took " << duration.count() << "ms." << std::endl;
	const BoundingBox & bbox = _scene->getBoundingBox();
	const float range = glm::length(bbox.getSize());
	_userCamera.frustum(0.01f*range, 5.0f*range);
	_userCamera.speed() = 0.2f*range;
	_ambientScreen.setSceneParameters(_scene->backgroundReflection->id, _scene->backgroundIrradiance);
	
	std::vector<GLuint> includedTextures = _gbuffer->textureIds();
	/// \todo clarify this.
	includedTextures.insert(includedTextures.begin()+2, _gbuffer->depthId());
	
	for(auto& light : _scene->lights){
		light->init(includedTextures);
	}
	checkGLError();
}

void DeferredRenderer::renderScene(){
	// Bind the full scene framebuffer.
	_gbuffer->bind();
	// Set screen viewport
	_gbuffer->setViewport();
	
	// Clear the depth buffer (we know we will draw everywhere, no need to clear color.
	glClear(GL_DEPTH_BUFFER_BIT);
	
	const glm::mat4 & view = _userCamera.view();
	const glm::mat4 & proj = _userCamera.projection();
	for(auto & object : _scene->objects){
		// Combine the three matrices.
		const glm::mat4 MV = view * object.model();
		const glm::mat4 MVP = proj * MV;
		// Compute the normal matrix
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));
		
		// Select the program (and shaders).
		switch (object.type()) {
			case Object::PBRParallax:
				glUseProgram(_parallaxProgram->id());
				// Upload the MVP matrix.
				glUniformMatrix4fv(_parallaxProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
				// Upload the projection matrix.
				glUniformMatrix4fv(_parallaxProgram->uniform("p"), 1, GL_FALSE, &proj[0][0]);
				// Upload the MV matrix.
				glUniformMatrix4fv(_parallaxProgram->uniform("mv"), 1, GL_FALSE, &MV[0][0]);
				// Upload the normal matrix.
				glUniformMatrix3fv(_parallaxProgram->uniform("normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
				break;
			case Object::PBRNoUVs:
				glUseProgram(_objectNoUVsProgram->id());
				// Upload the MVP matrix.
				glUniformMatrix4fv(_objectNoUVsProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
				// Upload the normal matrix.
				glUniformMatrix3fv(_objectNoUVsProgram->uniform("normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
			break;
			case Object::PBRRegular:
				glUseProgram(_objectProgram->id());
				// Upload the MVP matrix.
				glUniformMatrix4fv(_objectProgram->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
				// Upload the normal matrix.
				glUniformMatrix3fv(_objectProgram->uniform("normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
				break;
			default:
			
			break;
		}
		
		// Bind the textures.
		GLUtilities::bindTextures(object.textures());
		GLUtilities::drawMesh(*object.mesh());
		glUseProgram(0);
	}
	
	if(_debugVisualization){
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_CULL_FACE);
		for(auto & light : _scene->lights){
			light->drawDebug(_userCamera.view(), _userCamera.projection());
		}
		
		glEnable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	
	// No need to write the skybox depth to the framebuffer.
	glDepthMask(GL_FALSE);
	// Accept a depth of 1.0 (far plane).
	glDepthFunc(GL_LEQUAL);
	const Object * background = _scene->background.get();
	const Scene::Background mode = _scene->backgroundMode;
	
	if(mode == Scene::Background::SKYBOX){
		// Skybox.
		const glm::mat4 backgroundMVP = proj * view * background->model();
		// Draw background.
		glUseProgram(_skyboxProgram->id());
		// Upload the MVP matrix.
		glUniformMatrix4fv(_skyboxProgram->uniform("mvp"), 1, GL_FALSE, &backgroundMVP[0][0]);
		GLUtilities::bindTextures(background->textures());
		GLUtilities::drawMesh(*background->mesh());
		
	} else if(mode == Scene::Background::ATMOSPHERE){
		// Atmosphere screen quad.
		glUseProgram(_atmoProgram->id());
		// Revert the model to clip matrix, removing the translation part.
		const glm::mat4 worldToClipNoT = _userCamera.projection() * glm::mat4(glm::mat3(_userCamera.view()));
		const glm::mat4 clipToWorldNoT = glm::inverse(worldToClipNoT);
		const glm::vec3 & sunDir = dynamic_cast<const Sky *>(background)->direction();
		// Send and draw.
		glUniformMatrix4fv(_atmoProgram->uniform("clipToWorld"), 1, GL_FALSE, &clipToWorldNoT[0][0]);
		glUniform3fv(_atmoProgram->uniform("viewPos"), 1, &_userCamera.position()[0]);
		glUniform3fv(_atmoProgram->uniform("lightDirection"), 1, &sunDir[0]);
		GLUtilities::bindTextures(background->textures());
		GLUtilities::drawMesh(*background->mesh());
		
	} else {
		// Background color or 2D image.
		glUseProgram(_bgProgram->id());
		if(mode == Scene::Background::IMAGE){
			glUniform1i(_bgProgram->uniform("useTexture"), 1);
			GLUtilities::bindTextures(background->textures());
		} else {
			glUniform1i(_bgProgram->uniform("useTexture"), 0);
			glUniform3fv(_bgProgram->uniform("bgColor"), 1, &_scene->backgroundColor[0]);
		}
		GLUtilities::drawMesh(*background->mesh());
	}
	glUseProgram(0);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	
	// Unbind the full scene framebuffer.
	_gbuffer->unbind();
}

GLuint DeferredRenderer::renderPostprocess(const glm::vec2 & invRenderSize){
	
	if(_applyBloom){
		// --- Bloom selection pass ------
		_bloomFramebuffer->bind();
		_bloomFramebuffer->setViewport();
		glUseProgram(_bloomProgram->id());
		glUniform1f(_bloomProgram->uniform("luminanceTh"), _bloomTh);
		ScreenQuad::draw(_sceneFramebuffer->textureId());
		_bloomFramebuffer->unbind();
		
		// --- Bloom blur pass ------
		_blurBuffer->process(_bloomFramebuffer->textureId());
		
		// Draw the blurred bloom back into the scene framebuffer.
		_sceneFramebuffer->bind();
		_sceneFramebuffer->setViewport();
		glEnable(GL_BLEND);
		glUseProgram(_bloomCompositeProgram->id());
		glUniform1f(_bloomCompositeProgram->uniform("mixFactor"), _bloomMix);
		ScreenQuad::draw(_blurBuffer->textureId());
		glDisable(GL_BLEND);
		_sceneFramebuffer->unbind();
	}
	
	// --- Tonemapping pass ------
	_toneMappingFramebuffer->bind();
	_toneMappingFramebuffer->setViewport();
	glUseProgram(_toneMappingProgram->id());
	glUniform1f(_toneMappingProgram->uniform("customExposure"), _exposure);
	glUniform1i(_toneMappingProgram->uniform("apply"), _applyTonemapping);
	ScreenQuad::draw(_sceneFramebuffer->textureId());
	_toneMappingFramebuffer->unbind();
	GLuint currentResult = _toneMappingFramebuffer->textureId();
	
	
	if(_applyFXAA){
		// --- FXAA pass -------
		// Bind the post-processing framebuffer.
		_fxaaFramebuffer->bind();
		_fxaaFramebuffer->setViewport();
		glUseProgram(_fxaaProgram->id());
		glUniform2fv(_fxaaProgram->uniform("inverseScreenSize"), 1, &(invRenderSize[0]));
		ScreenQuad::draw(currentResult);
		_fxaaFramebuffer->unbind();
		currentResult = _fxaaFramebuffer->textureId();
	}
	
	return currentResult;
}

void DeferredRenderer::draw() {
	
	if(!_scene){
		glClearColor(0.2f,0.2f,0.2f,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}
	const glm::vec2 invRenderSize = 1.0f / _renderResolution;
	
	// --- Light pass -------
	if(_updateShadows){
		for(auto& light : _scene->lights){
			light->drawShadow(_scene->objects);
		}
	}
	
	// --- Scene pass -------
	renderScene();
	
	// --- SSAO pass
	glDisable(GL_DEPTH_TEST);
	if(_applySSAO){
		_ssaoPass->process(_userCamera.projection(), _gbuffer->depthId(), _gbuffer->textureId(int(TextureType::Normal)));
	} else {
		_ssaoPass->clear();
	}
	
	// --- Gbuffer composition pass
	_sceneFramebuffer->bind();
	_sceneFramebuffer->setViewport();
	_ambientScreen.draw(_userCamera.view(), _userCamera.projection());
	glEnable(GL_BLEND);
	for(auto& light : _scene->lights){
		light->draw(_userCamera.view(), _userCamera.projection(), invRenderSize);
	}
	glDisable(GL_BLEND);
	_sceneFramebuffer->unbind();
	
	// --- Post process passes -----
	const GLuint currentResult = renderPostprocess(invRenderSize);
	
	// --- Final pass -------
	// We now render a full screen quad in the default framebuffer, using sRGB space.
	glEnable(GL_FRAMEBUFFER_SRGB);
	glViewport(0, 0, GLsizei(_config.screenResolution[0]), GLsizei(_config.screenResolution[1]));
	glUseProgram(_finalProgram->id());
	ScreenQuad::draw(currentResult);
	glDisable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST);
	
	checkGLError();
}

void DeferredRenderer::update(){
	Renderer::update();
	// If no scene, no need to udpate the camera or the scene-specific UI.
	if(!_scene){
		return;
	}
	_userCamera.update();
	
	if(ImGui::Begin("Renderer")){
		ImGui::PushItemWidth(100);
		ImGui::InputFloat("Camera speed", &_userCamera.speed(), 0.1f, 1.0f);
		if(ImGui::InputFloat("Camera FOV", &_cameraFOV, 1.0f, 10.0f)){
			_userCamera.fov(_cameraFOV*float(M_PI)/180.0f);
		}
		ImGui::Combo("Camera mode", (int*)(&_userCamera.mode()), "FPS\0Turntable\0Joystick\0\0", 3);
		ImGui::PopItemWidth();
		if(ImGui::CollapsingHeader("Camera details")){
			ImGui::InputFloat3("Position", (float*)(&_userCamera.position()[0]));
			ImGui::InputFloat3("Center", (float*)(&_userCamera.center()[0]));
			ImGui::InputFloat3("Up", (float*)(&_userCamera.up()[0]));
			ImGui::InputFloat2("Clip planes", (float*)(&_userCamera.clippingPlanes()[0]));
			ImGui::Text("FoV (rad): %2.3f", _userCamera.fov());
		}
		ImGui::Separator();
		ImGui::PushItemWidth(100);
		if(ImGui::InputInt("Vertical res.", &_config.internalVerticalResolution, 50, 200)){
			resize(int(_config.screenResolution[0]), int(_config.screenResolution[1]));
		}
		ImGui::PopItemWidth();
		
		ImGui::Checkbox("Bloom", &_applyBloom);
		if(_applyBloom){
			ImGui::SliderFloat("Bloom mix", &_bloomMix, 0.0f, 1.5f);
			ImGui::SliderFloat("Bloom th.", &_bloomTh, 0.5f, 2.0f);
		}
		
		ImGui::Checkbox("SSAO", &_applySSAO); ImGui::SameLine(120);
		ImGui::Checkbox("FXAA", &_applyFXAA);
		ImGui::Checkbox("Tonemapping ", &_applyTonemapping);
		if(_applyTonemapping){
			ImGui::SliderFloat("Exposure", &_exposure, 0.1f, 10.0f);
		}
		
		ImGui::Separator();
		ImGui::ColorEdit3("Background color", &_scene->backgroundColor[0]);
		ImGui::Checkbox("Show debug lights", &_debugVisualization);
		ImGui::Checkbox("Update shadows", &_updateShadows);
		ImGui::Checkbox("Pause", &_paused);
		
		
	}
	ImGui::End();
}

void DeferredRenderer::physics(double fullTime, double frameTime){
	_userCamera.physics(frameTime);
	if(_scene && !_paused){
		_scene->update(fullTime, frameTime);
	}
}


void DeferredRenderer::clean() const {
	Renderer::clean();
	// Clean objects.
	_gbuffer->clean();
	_blurBuffer->clean();
	_ssaoPass->clean();
	_bloomFramebuffer->clean();
	_sceneFramebuffer->clean();
	_toneMappingFramebuffer->clean();
	_fxaaFramebuffer->clean();
	_blurBuffer->clean();
	if(_scene){
		_scene->clean();
	}
}


void DeferredRenderer::resize(unsigned int width, unsigned int height){
	Renderer::updateResolution(width, height);
	const unsigned int hWidth = (unsigned int)(_renderResolution[0] / 2.0f);
	const unsigned int hHeight = (unsigned int)(_renderResolution[1] / 2.0f);
	// Resize the framebuffers.
	_gbuffer->resize(_renderResolution);
	_ssaoPass->resize(hWidth, hHeight);
	_toneMappingFramebuffer->resize(_renderResolution);
	_fxaaFramebuffer->resize(_renderResolution);
	_sceneFramebuffer->resize(_renderResolution);
	_bloomFramebuffer->resize(_renderResolution);
	_blurBuffer->resize(hWidth, hHeight);
	checkGLError();
}



