#include "ForwardRenderer.hpp"
#include "scene/lights/Light.hpp"
#include "scene/Sky.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"
#include <chrono>

ForwardRenderer::ForwardRenderer(const glm::vec2 & resolution) :
	_lightDebugRenderer("object_basic_uniform") {

	_renderResolution		   = resolution;
	const int renderWidth	   = int(_renderResolution[0]);
	const int renderHeight	   = int(_renderResolution[1]);
	const int renderHalfWidth  = int(0.5f * _renderResolution[0]);
	const int renderHalfHeight = int(0.5f * _renderResolution[1]);

	// Other framebuffers.
	_ssaoPass		  = std::unique_ptr<SSAO>(new SSAO(renderHalfWidth, renderHalfHeight, 0.5f));
	_sceneFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, Layout::RGBA16F, true));

	_objectProgram		= Resources::manager().getProgram("object_forward");
	_objectNoUVsProgram = Resources::manager().getProgram("object_no_uv_forward");
	_parallaxProgram	= Resources::manager().getProgram("object_parallax_forward");

	_skyboxProgram = Resources::manager().getProgram("skybox_forward", "skybox_infinity", "skybox_basic");
	_bgProgram	   = Resources::manager().getProgram("background_infinity");
	_atmoProgram   = Resources::manager().getProgram("atmosphere_forward", "background_infinity", "atmosphere");

	_textureBrdf = Resources::manager().getTexture("brdf-precomputed", {Layout::RG32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::GPU);

	_renderResult = _sceneFramebuffer->textureId();
	checkGLError();
}

void ForwardRenderer::setScene(const std::shared_ptr<Scene> & scene) {
	// Do not accept a null scene.
	if(!scene) {
		return;
	}

	_scene = scene;
	_objectProgram->cacheUniformArray("shCoeffs", _scene->backgroundIrradiance);
	_objectNoUVsProgram->cacheUniformArray("shCoeffs", _scene->backgroundIrradiance);
	_parallaxProgram->cacheUniformArray("shCoeffs", _scene->backgroundIrradiance);
	_lightGPUData.reset(new ForwardLight(_scene->lights.size()));

	checkGLError();
}


void ForwardRenderer::renderScene(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos) {
	// Draw the scene.
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	_sceneFramebuffer->bind();
	_sceneFramebuffer->setViewport();
	GLUtilities::clearDepth(1.0f);

	const float cubeLod		= float(_scene->backgroundReflection->levels - 1);
	const glm::mat4 invView = glm::inverse(view);
	// Render all objects.
	for(auto & object : _scene->objects) {
		// Combine the three matrices.
		const glm::mat4 MV	= view * object.model();
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
				_parallaxProgram->uniform("inverseV", invView);
				_parallaxProgram->uniform("maxLod", cubeLod);
				_parallaxProgram->uniformBuffer("Lights", 0);
				_parallaxProgram->uniform("lightsCount", int(_lightGPUData->count()));
				break;
			case Object::PBRNoUVs:
				_objectNoUVsProgram->use();
				// Upload the MVP matrix.
				_objectNoUVsProgram->uniform("mvp", MVP);
				// Upload the MV matrix.
				_objectNoUVsProgram->uniform("mv", MV);
				// Upload the normal matrix.
				_objectNoUVsProgram->uniform("normalMatrix", normalMatrix);
				_objectNoUVsProgram->uniform("inverseV", invView);
				_objectNoUVsProgram->uniform("maxLod", cubeLod);
				_objectNoUVsProgram->uniformBuffer("Lights", 0);
				_objectNoUVsProgram->uniform("lightsCount", int(_lightGPUData->count()));
				break;
			case Object::PBRRegular:
				_objectProgram->use();
				// Upload the MVP matrix.
				_objectProgram->uniform("mvp", MVP);
				// Upload the MV matrix.
				_objectProgram->uniform("mv", MV);
				// Upload the normal matrix.
				_objectProgram->uniform("normalMatrix", normalMatrix);
				_objectProgram->uniform("inverseV", invView);
				_objectProgram->uniform("maxLod", cubeLod);
				_objectProgram->uniformBuffer("Lights", 0);
				_objectProgram->uniform("lightsCount", int(_lightGPUData->count()));
				break;
			default:
				break;
		}

		// Backface culling state.
		if(object.twoSided()) {
			glDisable(GL_CULL_FACE);
		}
		// Bind the lights.
		_lightGPUData->bind(0);
		// Bind the textures.
		GLUtilities::bindTextures(object.textures());
		GLUtilities::bindTexture(_textureBrdf, object.textures().size());
		GLUtilities::bindTexture(_scene->backgroundReflection, object.textures().size() + 1);
		GLUtilities::drawMesh(*object.mesh());
		// Restore state.
		glEnable(GL_CULL_FACE);
	}
	
	// Render all lights.
	if(_debugVisualization) {
		_lightDebugRenderer.updateCameraInfos(view, proj);
		glDisable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		for(auto & light : _scene->lights) {
			light->draw(_lightDebugRenderer);
		}
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_CULL_FACE);
	}

	// Render the backgound.
	renderBackground(view, proj, pos);
	_sceneFramebuffer->unbind();
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

void ForwardRenderer::renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos) {
	// No need to write the skybox depth to the framebuffer.
	glDepthMask(GL_FALSE);
	// Accept a depth of 1.0 (far plane).
	glDepthFunc(GL_LEQUAL);
	const Object * background	 = _scene->background.get();
	const Scene::Background mode = _scene->backgroundMode;

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
		const glm::mat4 worldToClipNoT = proj * glm::mat4(glm::mat3(view));
		const glm::mat4 clipToWorldNoT = glm::inverse(worldToClipNoT);
		const glm::vec3 & sunDir	   = dynamic_cast<const Sky *>(background)->direction();
		// Send and draw.
		_atmoProgram->uniform("clipToWorld", clipToWorldNoT);
		_atmoProgram->uniform("viewPos", pos);
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

void ForwardRenderer::draw(const Camera & camera) {

	const glm::mat4 & view = camera.view();
	const glm::mat4 & proj = camera.projection();
	const glm::vec3 & pos  = camera.position();
	// --- Update lights data ----
	_lightGPUData->updateCameraInfos(view, proj);
	_lightGPUData->updateShadowMapInfos(_shadowMode, 0.01f);
	for(const auto light : _scene->lights) {
		light->draw(*_lightGPUData);
	}
	_lightGPUData->upload();

	// --- Scene pass -------
	renderScene(view, proj, pos);

	// --- SSAO pass
	/*if(_applySSAO) {
		_ssaoPass->process(proj, _gbuffer->depthId(), _gbuffer->textureId(int(TextureType::Normal)));
	} else {
		_ssaoPass->clear();
	}*/
}

void ForwardRenderer::clean() {
	// Clean objects.
	_ssaoPass->clean();
	_sceneFramebuffer->clean();
}

void ForwardRenderer::resize(unsigned int width, unsigned int height) {
	_renderResolution[0] = float(width);
	_renderResolution[1] = float(height);
	//Renderer::updateResolution(width, height);
	const unsigned int hWidth  = uint(_renderResolution[0] / 2.0f);
	const unsigned int hHeight = uint(_renderResolution[1] / 2.0f);
	// Resize the framebuffers.
	_ssaoPass->resize(hWidth, hHeight);
	_sceneFramebuffer->resize(_renderResolution);
	checkGLError();
}
