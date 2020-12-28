#include "DeferredRenderer.hpp"
#include "scene/lights/Light.hpp"
#include "scene/Sky.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"

DeferredRenderer::DeferredRenderer(const glm::vec2 & resolution, ShadowMode mode, bool ssao) :
	_applySSAO(ssao), _shadowMode(mode) {

	const uint renderWidth	  = uint(resolution[0]);
	const uint renderHeight	  = uint(resolution[1]);

	// G-buffer setup.
	const Descriptor albedoDesc			= {Layout::RGBA16F, Filter::NEAREST_NEAREST, Wrap::CLAMP};
	const Descriptor normalDesc			= {Layout::RGB16F, Filter::NEAREST_NEAREST, Wrap::CLAMP};
	const Descriptor effectsDesc		= {Layout::RGB8, Filter::NEAREST_NEAREST, Wrap::CLAMP};
	const Descriptor depthDesc			= {Layout::DEPTH_COMPONENT32F, Filter::NEAREST_NEAREST, Wrap::CLAMP};
	const Descriptor lightDesc = {Layout::RGB16F, Filter::LINEAR_LINEAR, Wrap::CLAMP};

	const std::vector<Descriptor> descs = {albedoDesc, normalDesc, effectsDesc, depthDesc};
	_gbuffer							= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, descs, false, "G-buffer"));
	_ssaoPass							= std::unique_ptr<SSAO>(new SSAO(renderWidth, renderHeight, 2, 0.5f));
	_lightBuffer						= std::unique_ptr<Framebuffer>(new Framebuffer(TextureShape::D2, renderWidth, renderHeight, 1, 1, {lightDesc, depthDesc}, false, "Deferred lighting"));
	_preferredFormat.push_back(lightDesc);
	_needsDepth = false;

	_skyboxProgram		= Resources::manager().getProgram("skybox_gbuffer", "skybox_infinity", "skybox_gbuffer");
	_bgProgram			= Resources::manager().getProgram("background_gbuffer", "background_infinity", "background_gbuffer");
	_atmoProgram		= Resources::manager().getProgram("atmosphere_gbuffer", "background_infinity", "atmosphere_gbuffer");
	_parallaxProgram	= Resources::manager().getProgram("object_parallax_gbuffer");
	_objectProgram		= Resources::manager().getProgram("object_gbuffer");
	_emissiveProgram	= Resources::manager().getProgram("object_emissive_gbuffer");
	_transparentProgram = Resources::manager().getProgram("object_transparent_forward", "object_forward", "object_transparent_forward");

	// Lighting passes.
	_ambientScreen = std::unique_ptr<AmbientQuad>(new AmbientQuad(_gbuffer->texture(0), _gbuffer->texture(1),
		_gbuffer->texture(2), _gbuffer->depthBuffer(), _ssaoPass->texture()));
	_lightRenderer = std::unique_ptr<DeferredLight>(new DeferredLight(_gbuffer->texture(0), _gbuffer->texture(1), _gbuffer->depthBuffer(), _gbuffer->texture(2)));

	_textureBrdf = Resources::manager().getTexture("brdf-precomputed", {Layout::RG32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::GPU);
	checkGLError();
}

void DeferredRenderer::setScene(const std::shared_ptr<Scene> & scene) {
	// Do not accept a null scene.
	if(!scene) {
		return;
	}
	_scene = scene;
	_culler.reset(new Culler(_scene->objects));
	_fwdLightsGPU.reset(new ForwardLight(_scene->lights.size()));
	checkGLError();
}

void DeferredRenderer::renderOpaque(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj) {
	
	GLUtilities::setDepthState(true, TestFunction::LESS, true);
	GLUtilities::setBlendState(false);
	GLUtilities::setCullState(true, Faces::BACK);

	// Clear the depth buffer (we know we will draw everywhere, no need to clear color).
	GLUtilities::clearDepth(1.0f);

	// Scene objects.
	for(const long & objectId : visibles) {
		// Once we get a -1, there is no other object to render.
		if(objectId == -1){
			break;
		}
		const Object & object = _scene->objects[objectId];
		// Skip transparent objects.
		if(object.type() == Object::Transparent){
			continue;
		}
		
		// Combine the three matrices.
		const glm::mat4 MV  = view * object.model();
		const glm::mat4 MVP = proj * MV;
		// Compute the normal matrix
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));

		// Select the program (and shaders).
		switch(object.type()) {
			case Object::Parallax:
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
			case Object::Regular:
				_objectProgram->use();
				// Upload the MVP matrix.
				_objectProgram->uniform("mvp", MVP);
				// Upload the normal matrix.
				_objectProgram->uniform("normalMatrix", normalMatrix);
				_objectProgram->uniform("hasUV", object.useTexCoords());
				break;
			case Object::Emissive:
				_emissiveProgram->use();
				// Upload the MVP matrix.
				_emissiveProgram->uniform("mvp", MVP);
				// Are UV available.
				_emissiveProgram->uniform("hasUV", object.useTexCoords());
				break;
			default:
				break;
		}

		// Backface culling state.
		GLUtilities::setCullState(!object.twoSided(), Faces::BACK);

		// Bind the textures.
		GLUtilities::bindTextures(object.textures());
		GLUtilities::drawMesh(*object.mesh());
	}

}

void DeferredRenderer::renderTransparent(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj){

	const auto & shadowMaps = _fwdLightsGPU->shadowMaps();

	GLUtilities::setBlendState(true, BlendEquation::ADD, BlendFunction::ONE, BlendFunction::ONE_MINUS_SRC_ALPHA);
	GLUtilities::setDepthState(true, TestFunction::LEQUAL, true);
	GLUtilities::setCullState(true, Faces::BACK);

	_transparentProgram->use();

	// Update all shaders shared parameters.
	const LightProbe & environment = _scene->environment;
	const float cubeLod		= float(environment.map()->levels - 1);
	const glm::mat4 invView = glm::inverse(view);
	const glm::vec2 invScreenSize = 1.0f / glm::vec2(_lightBuffer->width(), _lightBuffer->height());
	// Update shared data.
	_transparentProgram->uniform("inverseV", invView);
	_transparentProgram->uniform("maxLod", cubeLod);
	_transparentProgram->uniform("cubemapPos", environment.position());
	_transparentProgram->uniform("cubemapCenter", environment.center());
	_transparentProgram->uniform("cubemapExtent", environment.extent());
	_transparentProgram->uniform("cubemapCosSin", environment.rotationCosSin());
	_transparentProgram->uniform("lightsCount", int(_fwdLightsGPU->count()));
	_transparentProgram->uniform("invScreenSize", invScreenSize);

	for(const long & objectId : visibles) {
		// Once we get a -1, there is no other object to render.
		if(objectId == -1){
			break;
		}

		const auto & object = _scene->objects[objectId];
		// Skip non transparent objects.
		if(object.type() != Object::Type::Transparent){
			continue;
		}

		// Combine the three matrices.
		const glm::mat4 MV	= view * object.model();
		const glm::mat4 MVP = proj * MV;
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));

		// Upload the matrices.
		_transparentProgram->uniform("hasUV", object.useTexCoords());
		_transparentProgram->uniform("mvp", MVP);
		_transparentProgram->uniform("mv", MV);
		_transparentProgram->uniform("normalMatrix", normalMatrix);

		// Bind the lights.
		GLUtilities::bindBuffer(_fwdLightsGPU->data(), 0);
		GLUtilities::bindBuffer(*_scene->environment.shCoeffs(), 1);
		// Bind the textures.
		GLUtilities::bindTextures(object.textures());
		GLUtilities::bindTexture(_textureBrdf, 4);
		GLUtilities::bindTexture(_scene->environment.map(), 5);
		// Bind available shadow maps.
		if(shadowMaps[0]){
			GLUtilities::bindTexture(shadowMaps[0], 6);
		}
		if(shadowMaps[1]){
			GLUtilities::bindTexture(shadowMaps[1], 7);
		}
		// No SSAO as the objects are not rendered in it.

		// To approximately handle two sided objects properly, draw the back faces first, then the front faces.
		// This won't solve all issues in case of concavities.
		if(object.twoSided()) {
			GLUtilities::setCullState(true, Faces::FRONT);
			GLUtilities::drawMesh(*object.mesh());
			GLUtilities::setCullState(true, Faces::BACK);
		}
		GLUtilities::drawMesh(*object.mesh());
	}
}

void DeferredRenderer::renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos){
	// Background.
	// No need to write the skybox depth to the framebuffer.
	// Accept a depth of 1.0 (far plane).
	GLUtilities::setDepthState(true, TestFunction::LEQUAL, false);
	GLUtilities::setBlendState(false);
	GLUtilities::setCullState(true, Faces::BACK);

	const Object * background	= _scene->background.get();
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
}

void DeferredRenderer::draw(const Camera & camera, Framebuffer & framebuffer, size_t layer) {

	const glm::mat4 & view = camera.view();
	const glm::mat4 & proj = camera.projection();
	const glm::vec3 & pos = camera.position();

	// Request list of visible objects from culler.
	const auto & visibles = _culler->cullAndSort(view, proj, pos);

	// Render opaque objects and the background to the Gbuffer.
	_gbuffer->bind();
	_gbuffer->setViewport();
	renderOpaque(visibles, view, proj);
	renderBackground(view, proj, pos);

	// SSAO pass
	if(_applySSAO) {
		_ssaoPass->process(proj, _gbuffer->depthBuffer(), _gbuffer->texture(int(TextureType::Normal)));
	} else {
		_ssaoPass->clear();
	}

	// Gbuffer lighting pass
	_lightRenderer->updateCameraInfos(view, proj);
	_lightRenderer->updateShadowMapInfos(_shadowMode, 0.002f);
	_lightBuffer->bind();
	_lightBuffer->setViewport();
	_ambientScreen->draw(view, proj, _scene->environment);

	for(auto & light : _scene->lights) {
		light->draw(*_lightRenderer);
	}
	// Blit the depth.
	GLUtilities::blitDepth(*_gbuffer, *_lightBuffer);
	
	// If transparent objects are present, prepare the forward pass.
	if(_scene->transparent()){
		// Update forward light data.
		_fwdLightsGPU->updateCameraInfos(view, proj);
		_fwdLightsGPU->updateShadowMapInfos(_shadowMode, 0.002f);
		for(const auto & light : _scene->lights) {
			light->draw(*_fwdLightsGPU);
		}
		_fwdLightsGPU->data().upload();
		// Now render transparent effects in a forward fashion.
		_lightBuffer->bind();
		_lightBuffer->setViewport();
		renderTransparent(visibles, view, proj);
	}

	// Copy to the final framebuffer.
	GLUtilities::blit(*_lightBuffer, framebuffer, 0, layer, Filter::NEAREST);

}

void DeferredRenderer::resize(unsigned int width, unsigned int height) {
	// Resize the framebuffers.
	const glm::vec2 nSize(width, height);
	_gbuffer->resize(nSize);
	_lightBuffer->resize(nSize);
	_ssaoPass->resize(width, height);
	checkGLError();
	
}

void DeferredRenderer::interface(){

	ImGui::Combo("Shadow technique", reinterpret_cast<int*>(&_shadowMode), "None\0Basic\0Variance\0\0");
	ImGui::Checkbox("SSAO", &_applySSAO);
	if(_applySSAO) {
		ImGui::SameLine();
		ImGui::Combo("Blur quality", reinterpret_cast<int*>(&_ssaoPass->quality()), "Low\0Medium\0High\0\0");
		ImGui::InputFloat("Radius", &_ssaoPass->radius(), 0.5f);
	}
	if(_culler){
		_culler->interface();
	}
	
}

const Framebuffer * DeferredRenderer::sceneDepth() const {
	return _lightBuffer.get();
}

const Texture * DeferredRenderer::sceneNormal() const {
	return _gbuffer->texture(1);
}
