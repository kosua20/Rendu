#include "ForwardRenderer.hpp"
#include "scene/lights/Light.hpp"
#include "scene/Sky.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"
#include <chrono>

ForwardRenderer::ForwardRenderer(const glm::vec2 & resolution) :
	_lightDebugRenderer("object_basic_uniform") {

	const int renderWidth	   = int(resolution[0]);
	const int renderHeight	   = int(resolution[1]);
	const int renderHalfWidth  = int(0.5f * resolution[0]);
	const int renderHalfHeight = int(0.5f * resolution[1]);

	// Framebuffers.
	const Descriptor descAmbient = {Layout::RGBA16F, Filter::LINEAR_NEAREST, Wrap::CLAMP};
	const Descriptor descDirect = {Layout::RGB16F, Filter::LINEAR_NEAREST, Wrap::CLAMP};
	const Descriptor descNormal = {Layout::RGB16F, Filter::LINEAR_NEAREST, Wrap::CLAMP};
	const Descriptor descDepth = {Layout::DEPTH_COMPONENT32F, Filter::NEAREST_NEAREST, Wrap::CLAMP};
	const std::vector<Descriptor> descs = { descAmbient, descDirect, descNormal, descDepth};
	_sceneFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, descs, true));
	_ssaoPass		  = std::unique_ptr<SSAO>(new SSAO(renderHalfWidth, renderHalfHeight, 0.5f));
	_preferredFormat.push_back(descAmbient);
	_needsDepth = false;

	_objectProgram		= Resources::manager().getProgram("object_forward");
	_objectNoUVsProgram = Resources::manager().getProgram("object_no_uv_forward");
	_parallaxProgram	= Resources::manager().getProgram("object_parallax_forward");
	_emissiveProgram	= Resources::manager().getProgram("object_emissive_forward");
	_compProgram	= Resources::manager().getProgram2D("composite_forward");

	_skyboxProgram = Resources::manager().getProgram("skybox_forward", "skybox_infinity", "skybox_forward");
	_bgProgram	   = Resources::manager().getProgram("background_forward", "background_infinity", "background_forward");
	_atmoProgram   = Resources::manager().getProgram("atmosphere_forward", "background_infinity", "atmosphere_forward");

	_textureBrdf = Resources::manager().getTexture("brdf-precomputed", {Layout::RG32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::GPU);

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
	_lightsGPU.reset(new ForwardLight(_scene->lights.size()));

	checkGLError();
}


void ForwardRenderer::renderScene(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos) {
	// Draw the scene.
	GLUtilities::setDepthState(true);
	GLUtilities::setCullState(true);
	_sceneFramebuffer->bind();
	_sceneFramebuffer->setViewport();
	GLUtilities::clearDepth(1.0f);
	GLUtilities::clearColor({0.0f,0.0f,0.0f,1.0f});

	const float cubeLod		= float(_scene->backgroundReflection->levels - 1);
	const glm::mat4 invView = glm::inverse(view);
	// Update shared data for the three programs.
	{
		_parallaxProgram->use();
		_parallaxProgram->uniform("p", proj);
		_parallaxProgram->uniform("inverseV", invView);
		_parallaxProgram->uniform("maxLod", cubeLod);
		_parallaxProgram->uniformBuffer("Lights", 0);
		_parallaxProgram->uniform("lightsCount", int(_lightsGPU->count()));
		_objectProgram->use();
		_objectProgram->uniform("inverseV", invView);
		_objectProgram->uniform("maxLod", cubeLod);
		_objectProgram->uniformBuffer("Lights", 0);
		_objectProgram->uniform("lightsCount", int(_lightsGPU->count()));
		_objectNoUVsProgram->use();
		_objectNoUVsProgram->uniform("inverseV", invView);
		_objectNoUVsProgram->uniform("maxLod", cubeLod);
		_objectNoUVsProgram->uniformBuffer("Lights", 0);
		_objectNoUVsProgram->uniform("lightsCount", int(_lightsGPU->count()));
	}
	const auto & shadowMaps = _lightsGPU->shadowMaps();

	// Build the camera frustum for culling.
	if(!_freezeFrustum){
		_frustumMat = proj*view;
	}
	const Frustum camFrustum(_frustumMat);
	// Scene objects.
	for(auto & object : _scene->objects) {
		// Check visibility.
		if(!camFrustum.intersects(object.boundingBox())){
			continue;
		}
		// Combine the three matrices.
		const glm::mat4 MV	= view * object.model();
		const glm::mat4 MVP = proj * MV;

		// Shortcut for emissive objects as their shader is quite different from other PBR shaders.
		if(object.type() == Object::Type::Emissive){
			_emissiveProgram->use();
			_emissiveProgram->uniform("mvp", MVP);
			_emissiveProgram->uniform("hasUV", !object.mesh()->texcoords.empty());
			if(object.twoSided()) {
				GLUtilities::setCullState(false);
			}
			// Bind the textures.
			GLUtilities::bindTextures(object.textures());
			GLUtilities::drawMesh(*object.mesh());
			GLUtilities::setCullState(true);
			continue;
		}


		// Compute the normal matrix
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));

		// Select the program (and shaders).
		Program * currentProgram = nullptr;
		switch(object.type()) {
			case Object::PBRParallax:
				currentProgram = _parallaxProgram;
				break;
			case Object::PBRNoUVs:
				currentProgram = _objectNoUVsProgram;
				break;
			case Object::PBRRegular:
				currentProgram = _objectProgram;
				break;
			default:
				Log::Error() << "Unsupported material type." << std::endl;
				continue;
				break;
		}

		currentProgram->use();
		// Upload the matrices.
		currentProgram->uniform("mvp", MVP);
		currentProgram->uniform("mv", MV);
		currentProgram->uniform("normalMatrix", normalMatrix);

		// Backface culling state.
		if(object.twoSided()) {
			GLUtilities::setCullState(false);
		}
		// Bind the lights.
		GLUtilities::bindBuffer(_lightsGPU->data(), 0);
		// Bind the textures.
		GLUtilities::bindTextures(object.textures());
		GLUtilities::bindTexture(_textureBrdf, 4);
		GLUtilities::bindTexture(_scene->backgroundReflection, 5);
		// Bind available shadow maps.
		if(shadowMaps[0]){
			GLUtilities::bindTexture(shadowMaps[0], 6);
		}
		if(shadowMaps[1]){
			GLUtilities::bindTexture(shadowMaps[1], 7);
		}
		GLUtilities::drawMesh(*object.mesh());
		// Restore state.
		GLUtilities::setCullState(true);
	}
	
	// Render all lights.
	if(_debugVisualization) {
		_lightDebugRenderer.updateCameraInfos(view, proj);
		GLUtilities::setCullState(false);
		GLUtilities::setPolygonState(PolygonMode::LINE, Faces::ALL);
		for(auto & light : _scene->lights) {
			light->draw(_lightDebugRenderer);
		}
		GLUtilities::setPolygonState(PolygonMode::FILL, Faces::ALL);
		GLUtilities::setCullState(true);
	}
	
	// Render the backgound.
	renderBackground(view, proj, pos);
	_sceneFramebuffer->unbind();
	GLUtilities::setDepthState(false);
	GLUtilities::setCullState(true);
}

void ForwardRenderer::renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos) {
	// No need to write the skybox depth to the framebuffer.
	// Accept a depth of 1.0 (far plane).
	GLUtilities::setDepthState(true, DepthEquation::LEQUAL, false);
	GLUtilities::setBlendState(false);
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

	GLUtilities::setDepthState(true, DepthEquation::LESS, true);
}

void ForwardRenderer::draw(const Camera & camera, Framebuffer & framebuffer, size_t layer) {

	const glm::mat4 & view = camera.view();
	const glm::mat4 & proj = camera.projection();
	const glm::vec3 & pos  = camera.position();
	// --- Update lights data ----
	_lightsGPU->updateCameraInfos(view, proj);
	_lightsGPU->updateShadowMapInfos(_shadowMode, 0.002f);
	for(const auto & light : _scene->lights) {
		light->draw(*_lightsGPU);
	}
	_lightsGPU->data().upload();

	// --- Scene pass
	renderScene(view, proj, pos);

	// --- SSAO pass
	if(_applySSAO) {
		_ssaoPass->process(proj, _sceneFramebuffer->depthId(), _sceneFramebuffer->textureId(2));
	} else {
		_ssaoPass->clear();
	}

	// --- Final composite pass
	framebuffer.bind(layer);
	framebuffer.setViewport();
	_compProgram->use();
	GLUtilities::setDepthState(false);
	GLUtilities::bindTexture(_sceneFramebuffer->textureId(0), 0);
	GLUtilities::bindTexture(_sceneFramebuffer->textureId(1), 1);
	GLUtilities::bindTexture(_ssaoPass->textureId(), 2);
	ScreenQuad::draw();
	framebuffer.unbind();
}

void ForwardRenderer::clean() {
	// Clean objects.
	_ssaoPass->clean();
	_sceneFramebuffer->clean();
}

void ForwardRenderer::resize(unsigned int width, unsigned int height) {
	// Resize the framebuffers.
	_ssaoPass->resize(width / 2, height / 2);
	_sceneFramebuffer->resize(glm::vec2(width, height));
	checkGLError();
}

void ForwardRenderer::interface(){
	ImGui::Checkbox("Show debug vis.", &_debugVisualization);
	ImGui::SameLine();
	ImGui::Checkbox("Freeze culling", &_freezeFrustum);
	ImGui::Combo("Shadow technique", reinterpret_cast<int*>(&_shadowMode), "None\0Basic\0Variance\0\0");
	ImGui::Checkbox("SSAO", &_applySSAO);
	if(_applySSAO) {
		ImGui::SameLine(120);
		ImGui::InputFloat("Radius", &_ssaoPass->radius(), 0.5f);
	}
}
