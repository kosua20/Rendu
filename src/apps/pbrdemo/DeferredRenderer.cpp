#include "DeferredRenderer.hpp"
#include "scene/lights/Light.hpp"
#include "scene/Sky.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"
#include <chrono>

DeferredRenderer::DeferredRenderer(RenderingConfig & config) :
	Renderer(config), _lightDebugRenderer("light_debug") {

	// Setup camera parameters.
	_userCamera.ratio(config.screenResolution[0] / config.screenResolution[1]);
	_cameraFOV = _userCamera.fov() * 180.0f / glm::pi<float>();
	_cplanes   = _userCamera.clippingPlanes();

	const int renderWidth	  = int(_renderResolution[0]);
	const int renderHeight	 = int(_renderResolution[1]);
	const int renderHalfWidth  = int(0.5f * _renderResolution[0]);
	const int renderHalfHeight = int(0.5f * _renderResolution[1]);

	// G-buffer setup.
	const Descriptor albedoDesc			= {Layout::RGBA16F, Filter::NEAREST_NEAREST, Wrap::CLAMP};
	const Descriptor normalDesc			= {Layout::RGB32F, Filter::NEAREST_NEAREST, Wrap::CLAMP};
	const Descriptor effectsDesc		= {Layout::RGB8, Filter::NEAREST_NEAREST, Wrap::CLAMP};
	const Descriptor depthDesc			= {Layout::DEPTH_COMPONENT32F, Filter::NEAREST_NEAREST, Wrap::CLAMP};
	const std::vector<Descriptor> descs = {albedoDesc, normalDesc, effectsDesc, depthDesc};
	_gbuffer							= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, descs, false));

	// Other framebuffers.
	_ssaoPass				= std::unique_ptr<SSAO>(new SSAO(renderHalfWidth, renderHalfHeight, 0.5f));
	_sceneFramebuffer		= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, Layout::RGBA16F, false));
	_bloomFramebuffer		= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, Layout::RGB16F, false));
	_toneMappingFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, Layout::RGB16F, false));
	_fxaaFramebuffer		= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, Layout::RGB16F, false));
	_blurBuffer				= std::unique_ptr<GaussianBlur>(new GaussianBlur(renderHalfWidth, renderHalfHeight, _bloomRadius, Layout::RGB16F));

	checkGLError();

	// GL options
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);

	_bloomProgram		   = Resources::manager().getProgram2D("bloom");
	_bloomCompositeProgram = Resources::manager().getProgram2D("bloom-composite");
	_toneMappingProgram	= Resources::manager().getProgram2D("tonemap");
	_fxaaProgram		   = Resources::manager().getProgram2D("fxaa");
	_finalProgram		   = Resources::manager().getProgram2D("final_screenquad");

	_skyboxProgram		= Resources::manager().getProgram("skybox_gbuffer", "skybox_infinity", "skybox_gbuffer");
	_bgProgram			= Resources::manager().getProgram("background_gbuffer", "background_infinity", "background_gbuffer");
	_atmoProgram		= Resources::manager().getProgram("atmosphere_gbuffer", "background_infinity", "atmosphere_gbuffer");
	_parallaxProgram	= Resources::manager().getProgram("object_parallax_gbuffer");
	_objectProgram		= Resources::manager().getProgram("object_gbuffer");
	_objectNoUVsProgram = Resources::manager().getProgram("object_no_uv_gbuffer");

	// Lighting passes.
	_ambientScreen = std::unique_ptr<AmbientQuad>(new AmbientQuad(_gbuffer->textureId(0), _gbuffer->textureId(1),
		_gbuffer->textureId(2), _gbuffer->depthId(), _ssaoPass->textureId()));
	_lightRenderer = std::unique_ptr<DeferredLight>(new DeferredLight(_gbuffer->textureId(0), _gbuffer->textureId(1), _gbuffer->depthId(), _gbuffer->textureId(2)));
	checkGLError();
}

void DeferredRenderer::setScene(const std::shared_ptr<Scene> & scene) {
	_scene = scene;
	if(!scene) {
		return;
	}

	_scene->init(Storage::GPU);

	_userCamera.apply(_scene->viewpoint());
	_userCamera.ratio(_renderResolution[0] / _renderResolution[1]);
	const BoundingBox & bbox = _scene->boundingBox();
	const float range		 = glm::length(bbox.getSize());
	_userCamera.frustum(0.01f * range, 5.0f * range);
	_userCamera.speed() = 0.2f * range;
	_cplanes			= _userCamera.clippingPlanes();
	_cameraFOV			= _userCamera.fov() * 180.0f / glm::pi<float>();
	_ambientScreen->setSceneParameters(_scene->backgroundReflection, _scene->backgroundIrradiance);
	
	// Delete existing shadow maps.
	for(auto & map : _shadowMaps){
		map->clean();
	}
	_shadowMaps.clear();
	// Allocate shadow maps.
	for(auto & light : scene->lights){
		if(!light->castsShadow()){
			continue;
		}
		if(auto pLight = std::dynamic_pointer_cast<PointLight>(light)){
			_shadowMaps.emplace_back(new ShadowMapCube(pLight, 512));
		} else {
			_shadowMaps.emplace_back(new ShadowMap2D(light, glm::vec2(512)));
		}
	}
	checkGLError();
}

void DeferredRenderer::renderScene() {
	// Bind the full scene framebuffer.
	_gbuffer->bind();
	// Set screen viewport
	_gbuffer->setViewport();
	// Clear the depth buffer (we know we will draw everywhere, no need to clear color).
	GLUtilities::clearDepth(1.0f);

	// Scene objects.
	const glm::mat4 & view = _userCamera.view();
	const glm::mat4 & proj = _userCamera.projection();
	for(auto & object : _scene->objects) {
		// Combine the three matrices.
		const glm::mat4 MV  = view * object.model();
		const glm::mat4 MVP = proj * MV;
		// Compute the normal matrix
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));

		// Select the program (and shaders).
		switch(object.type()) {
			case Object::PBRParallax:
				_parallaxProgram->use();
				// Upload the MVP matrix.
				_parallaxProgram->uniform("mvp", MVP);
				// Upload the projection matrix.
				_parallaxProgram->uniform("p", proj);
				// Upload the MV matrix.
				_parallaxProgram->uniform("mv", MV);
				// Upload the normal matrix.
				_parallaxProgram->uniform("normalMatrix", normalMatrix);
				break;
			case Object::PBRNoUVs:
				_objectNoUVsProgram->use();
				// Upload the MVP matrix.
				_objectNoUVsProgram->uniform("mvp", MVP);
				// Upload the normal matrix.
				_objectNoUVsProgram->uniform("normalMatrix", normalMatrix);
				break;
			case Object::PBRRegular:
				_objectProgram->use();
				// Upload the MVP matrix.
				_objectProgram->uniform("mvp", MVP);
				// Upload the normal matrix.
				_objectProgram->uniform("normalMatrix", normalMatrix);
				break;
			default:
				break;
		}

		// Backface culling state.
		if(object.twoSided()) {
			glDisable(GL_CULL_FACE);
		}

		// Bind the textures.
		GLUtilities::bindTextures(object.textures());
		GLUtilities::drawMesh(*object.mesh());
		// Restore state.
		glEnable(GL_CULL_FACE);
	}
	
	// Lights wireframe debug.
	if(_debugVisualization){
		_lightDebugRenderer.updateCameraInfos(_userCamera.view(), _userCamera.projection());
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_CULL_FACE);
		for(const auto & light : _scene->lights){
			light->draw(_lightDebugRenderer);
		}
		glEnable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	renderBackground();

	// Unbind the full scene framebuffer.
	_gbuffer->unbind();
}

void DeferredRenderer::renderBackground(){
	// Background.
	// No need to write the skybox depth to the framebuffer.
	glDepthMask(GL_FALSE);
	// Accept a depth of 1.0 (far plane).
	glDepthFunc(GL_LEQUAL);
	const Object * background	= _scene->background.get();
	const Scene::Background mode = _scene->backgroundMode;
	const glm::mat4 & view = _userCamera.view();
	const glm::mat4 & proj = _userCamera.projection();
	
	if(mode == Scene::Background::SKYBOX) {
		// Skybox.
		const glm::mat4 backgroundMVP = proj * view * background->model();
		// Draw background.
		_skyboxProgram->use();
		// Upload the MVP matrix.
		_skyboxProgram->uniform("mvp", backgroundMVP);
		GLUtilities::bindTextures(background->textures());
		GLUtilities::drawMesh(*background->mesh());
		
	} else if(mode == Scene::Background::ATMOSPHERE) {
		// Atmosphere screen quad.
		_atmoProgram->use();
		// Revert the model to clip matrix, removing the translation part.
		const glm::mat4 worldToClipNoT = _userCamera.projection() * glm::mat4(glm::mat3(_userCamera.view()));
		const glm::mat4 clipToWorldNoT = glm::inverse(worldToClipNoT);
		const glm::vec3 & sunDir	   = dynamic_cast<const Sky *>(background)->direction();
		// Send and draw.
		_atmoProgram->uniform("clipToWorld", clipToWorldNoT);
		_atmoProgram->uniform("viewPos", _userCamera.position());
		_atmoProgram->uniform("lightDirection", sunDir);
		GLUtilities::bindTextures(background->textures());
		GLUtilities::drawMesh(*background->mesh());
		
	} else {
		// Background color or 2D image.
		_bgProgram->use();
		if(mode == Scene::Background::IMAGE) {
			_bgProgram->uniform("useTexture", 1);
			GLUtilities::bindTextures(background->textures());
		} else {
			_bgProgram->uniform("useTexture", 0);
			_bgProgram->uniform("bgColor", _scene->backgroundColor);
		}
		GLUtilities::drawMesh(*background->mesh());
	}
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
}

const Texture * DeferredRenderer::renderPostprocess(const glm::vec2 & invRenderSize) const {

	if(_applyBloom) {
		// --- Bloom selection pass ------
		_bloomFramebuffer->bind();
		_bloomFramebuffer->setViewport();
		_bloomProgram->use();
		_bloomProgram->uniform("luminanceTh", _bloomTh);
		ScreenQuad::draw(_sceneFramebuffer->textureId());
		_bloomFramebuffer->unbind();

		// --- Bloom blur pass ------
		_blurBuffer->process(_bloomFramebuffer->textureId());

		// Draw the blurred bloom back into the scene framebuffer.
		_sceneFramebuffer->bind();
		_sceneFramebuffer->setViewport();
		glEnable(GL_BLEND);
		_bloomCompositeProgram->use();
		_bloomCompositeProgram->uniform("mixFactor", _bloomMix);
		ScreenQuad::draw(_blurBuffer->textureId());
		glDisable(GL_BLEND);
		_sceneFramebuffer->unbind();
	}

	// --- Tonemapping pass ------
	_toneMappingFramebuffer->bind();
	_toneMappingFramebuffer->setViewport();
	_toneMappingProgram->use();
	_toneMappingProgram->uniform("customExposure", _exposure);
	_toneMappingProgram->uniform("apply", _applyTonemapping);
	ScreenQuad::draw(_sceneFramebuffer->textureId());
	_toneMappingFramebuffer->unbind();
	const Texture * currentResult = _toneMappingFramebuffer->textureId();

	if(_applyFXAA) {
		// --- FXAA pass -------
		// Bind the post-processing framebuffer.
		_fxaaFramebuffer->bind();
		_fxaaFramebuffer->setViewport();
		_fxaaProgram->use();
		_fxaaProgram->uniform("inverseScreenSize", invRenderSize);
		ScreenQuad::draw(currentResult);
		_fxaaFramebuffer->unbind();
		currentResult = _fxaaFramebuffer->textureId();
	}

	return currentResult;
}

void DeferredRenderer::draw() {

	if(!_scene) {
		GLUtilities::clearColorAndDepth({0.2f, 0.2f, 0.2f, 1.0f}, 1.0f);
		return;
	}
	const glm::vec2 invRenderSize = 1.0f / _renderResolution;

	// --- Light pass -------
	if(_updateShadows) {
		for(const auto & map : _shadowMaps){
			map->draw(*_scene);
		}
	}

	// --- Scene pass -------
	renderScene();

	// --- SSAO pass
	glDisable(GL_DEPTH_TEST);
	if(_applySSAO) {
		_ssaoPass->process(_userCamera.projection(), _gbuffer->depthId(), _gbuffer->textureId(int(TextureType::Normal)));
	} else {
		_ssaoPass->clear();
	}

	// --- Gbuffer composition pass
	_lightRenderer->updateCameraInfos(_userCamera.view(), _userCamera.projection());
	_sceneFramebuffer->bind();
	_sceneFramebuffer->setViewport();
	_ambientScreen->draw(_userCamera.view(), _userCamera.projection());
	glEnable(GL_BLEND);
	for(auto & light : _scene->lights) {
		light->draw(*_lightRenderer);
	}
	glDisable(GL_BLEND);
	_sceneFramebuffer->unbind();

	// --- Post process passes -----
	const Texture * currentResult = renderPostprocess(invRenderSize);

	// --- Final pass -------
	// We now render a full screen quad in the default framebuffer, using sRGB space.
	glEnable(GL_FRAMEBUFFER_SRGB);
	GLUtilities::setViewport(0, 0, int(_config.screenResolution[0]), int(_config.screenResolution[1]));
	_finalProgram->use();
	ScreenQuad::draw(currentResult);
	glDisable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST);

	checkGLError();
}

void DeferredRenderer::update() {
	Renderer::update();
	// If no scene, no need to udpate the camera or the scene-specific UI.
	if(!_scene) {
		return;
	}
	_userCamera.update();

	if(ImGui::Begin("Renderer")) {

		ImGui::PushItemWidth(110);
		ImGui::Combo("Camera mode", reinterpret_cast<int *>(&_userCamera.mode()), "FPS\0Turntable\0Joystick\0\0", 3);
		ImGui::InputFloat("Camera speed", &_userCamera.speed(), 0.1f, 1.0f);
		if(ImGui::InputFloat("Camera FOV", &_cameraFOV, 1.0f, 10.0f)) {
			_userCamera.fov(_cameraFOV * glm::pi<float>() / 180.0f);
		}
		ImGui::PopItemWidth();

		if(ImGui::DragFloat2("Planes", static_cast<float *>(&_cplanes[0]))) {
			_userCamera.frustum(_cplanes[0], _cplanes[1]);
		}

		if(ImGui::Button("Copy camera", ImVec2(104, 0))) {
			const std::string camDesc = _userCamera.encode();
			ImGui::SetClipboardText(camDesc.c_str());
		}
		ImGui::SameLine();
		if(ImGui::Button("Paste camera", ImVec2(104, 0))) {
			const std::string camDesc(ImGui::GetClipboardText());
			const auto cameraCode = Codable::parse(camDesc);
			if(!cameraCode.empty()) {
				_userCamera.decode(cameraCode[0]);
				_cameraFOV = _userCamera.fov() * 180.0f / glm::pi<float>();
				_cplanes   = _userCamera.clippingPlanes();
			}
		}

		ImGui::Separator();
		ImGui::PushItemWidth(110);
		if(ImGui::InputInt("Vertical res.", &_config.internalVerticalResolution, 50, 200)) {
			resize(int(_config.screenResolution[0]), int(_config.screenResolution[1]));
		}

		ImGui::Checkbox("Bloom  ", &_applyBloom);
		if(_applyBloom) {
			ImGui::SameLine(120);
			ImGui::SliderFloat("Th.##Bloom", &_bloomTh, 0.5f, 2.0f);

			ImGui::PushItemWidth(80);
			if(ImGui::InputInt("Rad.##Bloom", &_bloomRadius, 1, 10)) {
				_blurBuffer.reset(new GaussianBlur(int(_renderResolution[0] / 2), int(_renderResolution[1] / 2), _bloomRadius, Layout::RGB16F));
			}
			ImGui::PopItemWidth();
			ImGui::SameLine(120);
			ImGui::SliderFloat("Mix##Bloom", &_bloomMix, 0.0f, 1.5f);
		}

		ImGui::Checkbox("SSAO", &_applySSAO);
		if(_applySSAO) {
			ImGui::SameLine(120);
			ImGui::InputFloat("Radius", &_ssaoPass->radius(), 0.5f);
		}

		ImGui::Checkbox("Tonemap ", &_applyTonemapping);
		if(_applyTonemapping) {
			ImGui::SameLine(120);
			ImGui::SliderFloat("Exposure", &_exposure, 0.1f, 10.0f);
		}
		ImGui::Checkbox("FXAA", &_applyFXAA);

		ImGui::Separator();
		ImGui::Checkbox("Debug", &_debugVisualization);
		ImGui::SameLine();
		ImGui::Checkbox("Pause", &_paused);
		ImGui::SameLine();
		ImGui::Checkbox("Update shadows", &_updateShadows);
		ImGui::PopItemWidth();
		ImGui::ColorEdit3("Background", &_scene->backgroundColor[0], ImGuiColorEditFlags_Float);
	}
	ImGui::End();
}

void DeferredRenderer::physics(double fullTime, double frameTime) {
	_userCamera.physics(frameTime);
	if(_scene && !_paused) {
		_scene->update(fullTime, frameTime);
	}
}

void DeferredRenderer::clean() {
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
	for(auto & map : _shadowMaps){
		map->clean();
	}
}

void DeferredRenderer::resize(unsigned int width, unsigned int height) {
	Renderer::updateResolution(width, height);
	const unsigned int hWidth  = uint(_renderResolution[0] / 2.0f);
	const unsigned int hHeight = uint(_renderResolution[1] / 2.0f);
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
