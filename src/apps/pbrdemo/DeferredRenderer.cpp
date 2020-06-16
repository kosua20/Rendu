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
	const Descriptor desc = {Layout::RGB16F, Filter::LINEAR_LINEAR, Wrap::CLAMP};

	const std::vector<Descriptor> descs = {albedoDesc, normalDesc, effectsDesc, depthDesc};
	_gbuffer							= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, descs, false, "G-buffer"));
	_ssaoPass							= std::unique_ptr<SSAO>(new SSAO(renderWidth, renderHeight, 2, 0.5f));
	_lightBuffer						= std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, desc, false, "Deferred lighting"));
	_preferredFormat.push_back(desc);
	_needsDepth = false;

	_skyboxProgram		= Resources::manager().getProgram("skybox_gbuffer", "skybox_infinity", "skybox_gbuffer");
	_bgProgram			= Resources::manager().getProgram("background_gbuffer", "background_infinity", "background_gbuffer");
	_atmoProgram		= Resources::manager().getProgram("atmosphere_gbuffer", "background_infinity", "atmosphere_gbuffer");
	_parallaxProgram	= Resources::manager().getProgram("object_parallax_gbuffer");
	_objectProgram		= Resources::manager().getProgram("object_gbuffer");
	_objectNoUVsProgram = Resources::manager().getProgram("object_no_uv_gbuffer");
	_emissiveProgram	= Resources::manager().getProgram("object_emissive_gbuffer");

	// Lighting passes.
	_ambientScreen = std::unique_ptr<AmbientQuad>(new AmbientQuad(_gbuffer->texture(0), _gbuffer->texture(1),
		_gbuffer->texture(2), _gbuffer->depthBuffer(), _ssaoPass->texture()));
	_lightRenderer = std::unique_ptr<DeferredLight>(new DeferredLight(_gbuffer->texture(0), _gbuffer->texture(1), _gbuffer->depthBuffer(), _gbuffer->texture(2)));
	checkGLError();
}

void DeferredRenderer::setScene(const std::shared_ptr<Scene> & scene) {
	// Do not accept a null scene.
	if(!scene) {
		return;
	}
	_scene = scene;
	checkGLError();
}

void DeferredRenderer::renderScene(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos) {
	GLUtilities::setDepthState(true);
	
	// Bind the full scene framebuffer.
	_gbuffer->bind();
	// Set screen viewport
	_gbuffer->setViewport();
	// Clear the depth buffer (we know we will draw everywhere, no need to clear color).
	GLUtilities::clearDepth(1.0f);

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
			case Object::Emissive:
				_emissiveProgram->use();
				// Upload the MVP matrix.
				_emissiveProgram->uniform("mvp", MVP);
				// Are UV available. Note: we might want to decouple UV use from their existence.
				_emissiveProgram->uniform("hasUV", !object.mesh()->texcoords.empty());
				break;
			default:
				break;
		}

		// Backface culling state.
		if(object.twoSided()) {
			GLUtilities::setCullState(false);
		}

		// Bind the textures.
		GLUtilities::bindTextures(object.textures());
		GLUtilities::drawMesh(*object.mesh());
		// Restore state.
		GLUtilities::setCullState(true);
	}
	
	renderBackground(view, proj, pos);

	// Unbind the full scene framebuffer.
	_gbuffer->unbind();
	GLUtilities::setDepthState(false);
}

void DeferredRenderer::renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos){
	// Background.
	// No need to write the skybox depth to the framebuffer.
	// Accept a depth of 1.0 (far plane).
	GLUtilities::setDepthState(true, DepthEquation::LEQUAL, false);
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
	GLUtilities::setDepthState(true, DepthEquation::LESS, true);
}

void DeferredRenderer::draw(const Camera & camera, Framebuffer & framebuffer, size_t layer) {

	const glm::mat4 & view = camera.view();
	const glm::mat4 & proj = camera.projection();
	const glm::vec3 & pos = camera.position();

	// --- Scene pass -------
	renderScene(view, proj, pos);

	// --- SSAO pass
	if(_applySSAO) {
		_ssaoPass->process(proj, _gbuffer->depthBuffer(), _gbuffer->texture(int(TextureType::Normal)));
	} else {
		_ssaoPass->clear();
	}

	// --- Gbuffer composition pass
	_lightRenderer->updateCameraInfos(view, proj);
	_lightRenderer->updateShadowMapInfos(_shadowMode, 0.002f);
	_lightBuffer->bind();
	_lightBuffer->setViewport();
	_ambientScreen->draw(view, proj, _scene->environment.map(), *_scene->environment.shCoeffs());
	GLUtilities::setBlendState(true, BlendEquation::ADD, BlendFunction::ONE, BlendFunction::ONE);
	for(auto & light : _scene->lights) {
		light->draw(*_lightRenderer);
	}
	GLUtilities::setBlendState(false);
	_lightBuffer->unbind();
	// Copy to the final framebuffer.
	GLUtilities::blit(*_lightBuffer, framebuffer, 0, layer, Filter::NEAREST);

}

void DeferredRenderer::resize(unsigned int width, unsigned int height) {
	// Resize the framebuffers.
	_gbuffer->resize(glm::vec2(width, height));
	_lightBuffer->resize(glm::vec2(width, height));
	_ssaoPass->resize(width, height);
	checkGLError();
	
}

void DeferredRenderer::interface(){
	ImGui::Checkbox("Freeze culling", &_freezeFrustum);
	ImGui::Combo("Shadow technique", reinterpret_cast<int*>(&_shadowMode), "None\0Basic\0Variance\0\0");
	ImGui::Checkbox("SSAO", &_applySSAO);
	if(_applySSAO) {
		ImGui::SameLine();
		ImGui::Combo("Blur quality", reinterpret_cast<int*>(&_ssaoPass->quality()), "Low\0Medium\0High\0\0");
		ImGui::InputFloat("Radius", &_ssaoPass->radius(), 0.5f);
	}


}
