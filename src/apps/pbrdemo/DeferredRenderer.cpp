#include "DeferredRenderer.hpp"
#include "scene/lights/Light.hpp"
#include "scene/Sky.hpp"
#include "system/System.hpp"
#include "graphics/GPU.hpp"


DeferredRenderer::DeferredRenderer(const glm::vec2 & resolution, bool ssao, const std::string & name) :
	Renderer(name), _sceneAlbedo(_name + " albedo"), _sceneNormal(_name + " normals"), _sceneEffects(_name + " effects"), _sceneDepth(name + " depth"), _lighting(name +" lighting"), _indirectLighting(name + " indirect Lighting"), _depthCopy(name + " depth copy"), _applySSAO(ssao) {

	const uint renderWidth	  = uint(resolution[0]);
	const uint renderHeight	  = uint(resolution[1]);

	// G-buffer setup.
	const Layout albedoDesc			= Layout::RGBA8;
	const Layout normalDesc			= Layout::A2_RGB10;
	const Layout effectsDesc		= Layout::RGBA8;
	const Layout depthDesc			= Layout::DEPTH_COMPONENT32F;
	const Layout lightDesc 			= Layout::RGBA16F;
	_colorFormat = lightDesc;

	// Setup Gbuffer and lighting attachments.
	_sceneAlbedo.setupAsDrawable(albedoDesc, renderWidth, renderHeight);
	_sceneNormal.setupAsDrawable(normalDesc, renderWidth, renderHeight);
	_sceneEffects.setupAsDrawable(effectsDesc, renderWidth, renderHeight);
	_sceneDepth.setupAsDrawable(depthDesc, renderWidth, renderHeight);
	_lighting.setupAsDrawable(lightDesc, renderWidth, renderHeight);
	_indirectLighting.setupAsDrawable(lightDesc, renderWidth, renderHeight);
	_depthCopy.setupAsDrawable(depthDesc, renderWidth, renderHeight);

	_ssaoPass			= std::unique_ptr<SSAO>(new SSAO(renderWidth, renderHeight, 2, 0.5f, _name));

	_skyboxProgram		= Resources::manager().getProgram("skybox_gbuffer", "skybox_infinity", "skybox_gbuffer");
	_bgProgram			= Resources::manager().getProgram("background_gbuffer", "background_infinity", "background_gbuffer");
	_atmoProgram		= Resources::manager().getProgram("atmosphere_gbuffer", "background_infinity", "atmosphere_gbuffer");
	_parallaxProgram	= Resources::manager().getProgram("object_parallax_gbuffer");
	_objectProgram		= Resources::manager().getProgram("object_gbuffer");
	_clearCoatProgram	= Resources::manager().getProgram("object_clearcoat_gbuffer", "object_gbuffer", "object_clearcoat_gbuffer");
	_anisotropicProgram	= Resources::manager().getProgram("object_anisotropic_gbuffer", "object_gbuffer", "object_anisotropic_gbuffer");
	_sheenProgram		= Resources::manager().getProgram("object_sheen_gbuffer", "object_gbuffer", "object_sheen_gbuffer");
	_iridescentProgram	= Resources::manager().getProgram("object_iridescent_gbuffer", "object_gbuffer", "object_iridescent_gbuffer");
	_subsurfaceProgram	= Resources::manager().getProgram("object_subsurface_gbuffer", "object_gbuffer", "object_subsurface_gbuffer");
	_emissiveProgram	= Resources::manager().getProgram("object_emissive_gbuffer", "object_gbuffer", "object_emissive_gbuffer");
	_transparentProgram = Resources::manager().getProgram("object_transparent_forward", "object_forward", "object_transparent_forward");
	_transpIridProgram  = Resources::manager().getProgram("object_transparent_irid_forward", "object_forward", "object_transparent_irid_forward");

	// Lighting passes.
	_lightRenderer = std::unique_ptr<DeferredLight>(new DeferredLight(&_sceneAlbedo, &_sceneNormal, &_sceneDepth, &_sceneEffects));
	_probeRenderer = std::unique_ptr<DeferredProbe>(new DeferredProbe(&_sceneAlbedo, &_sceneNormal, &_sceneEffects, &_sceneDepth, _ssaoPass->texture()));
	_probeNormalization = Resources::manager().getProgram2D("probe_normalization");

	_textureBrdf = Resources::manager().getTexture("brdf-precomputed", Layout::RGBA16F, Storage::GPU);
}

void DeferredRenderer::setScene(const std::shared_ptr<Scene> & scene) {
	// Do not accept a null scene.
	if(!scene) {
		return;
	}
	_scene = scene;
	_culler.reset(new Culler(_scene->objects));
	_fwdLightsGPU.reset(new ForwardLight(_scene->lights.size()));
	_fwdProbesGPU.reset(new ForwardProbe(_scene->probes.size()));
}

void DeferredRenderer::renderOpaque(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj) {

	GPUMarker marker("Opaque objects");

	GPU::setDepthState(true, TestFunction::LESS, true);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);

	// Scene objects.
	for(const long & objectId : visibles) {
		// Once we get a -1, there is no other object to render.
		if(objectId == -1){
			break;
		}
		const Object & object = _scene->objects[objectId];
		const Material& material = object.material();
		// Skip transparent objects.
		if((material.type() == Material::Type::Transparent) || (material.type() == Material::Type::TransparentIrid)){
			continue;
		}
		
		// Combine the three matrices.
		const glm::mat4 MV  = view * object.model();
		const glm::mat4 MVP = proj * MV;
		// Compute the normal matrix
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));

		Program* program = nullptr;

		// Select the program (and shaders).
		switch(material.type()) {
			case Material::Parallax:
				_parallaxProgram->use();
				program = _parallaxProgram;
				// Upload the MVP matrix.
				_parallaxProgram->uniform("mvp", MVP);
				// Upload the projection matrix.
				_parallaxProgram->uniform("p", proj);
				// Upload the MV matrix.
				_parallaxProgram->uniform("mv", MV);
				// Upload the normal matrix.
				_parallaxProgram->uniform("normalMatrix", glm::mat4(normalMatrix));
				break;
			case Material::Regular:
				_objectProgram->use();
				program = _objectProgram;
				// Upload the MVP matrix.
				_objectProgram->uniform("mvp", MVP);
				// Upload the normal matrix.
				_objectProgram->uniform("normalMatrix", glm::mat4(normalMatrix));
				_objectProgram->uniform("hasUV", object.useTexCoords());
				break;
			case Material::Clearcoat:
				_clearCoatProgram->use();
				program = _clearCoatProgram;
				// Upload the MVP matrix.
				_clearCoatProgram->uniform("mvp", MVP);
				// Upload the normal matrix.
				_clearCoatProgram->uniform("normalMatrix", glm::mat4(normalMatrix));
				_clearCoatProgram->uniform("hasUV", object.useTexCoords());
				break;
			case Material::Anisotropic:
				_anisotropicProgram->use();
				program = _anisotropicProgram;
				// Upload the MVP matrix.
				_anisotropicProgram->uniform("mvp", MVP);
				// Upload the normal matrix.
				_anisotropicProgram->uniform("normalMatrix", glm::mat4(normalMatrix));
				_anisotropicProgram->uniform("hasUV", object.useTexCoords());
				break;
			case Material::Sheen:
				_sheenProgram->use();
				program = _sheenProgram;
				// Upload the MVP matrix.
				_sheenProgram->uniform("mvp", MVP);
				// Upload the normal matrix.
				_sheenProgram->uniform("normalMatrix", glm::mat4(normalMatrix));
				_sheenProgram->uniform("hasUV", object.useTexCoords());
				break;
			case Material::Iridescent:
				_iridescentProgram->use();
				program = _iridescentProgram;
				// Upload the MVP matrix.
				_iridescentProgram->uniform("mvp", MVP);
				// Upload the normal matrix.
				_iridescentProgram->uniform("normalMatrix", glm::mat4(normalMatrix));
				_iridescentProgram->uniform("hasUV", object.useTexCoords());
				break;
			case Material::Subsurface:
				_subsurfaceProgram->use();
				program = _subsurfaceProgram;
				// Upload the MVP matrix.
				_subsurfaceProgram->uniform("mvp", MVP);
				// Upload the normal matrix.
				_subsurfaceProgram->uniform("normalMatrix", glm::mat4(normalMatrix));
				_subsurfaceProgram->uniform("hasUV", object.useTexCoords());
				break;
			case Material::Emissive:
				_emissiveProgram->use();
				program = _emissiveProgram;
				// Upload the MVP matrix.
				_emissiveProgram->uniform("mvp", MVP);
				_emissiveProgram->uniform("normalMatrix", glm::mat4(normalMatrix));
				// Are UV available.
				_emissiveProgram->uniform("hasUV", object.useTexCoords());
				break;
			default:
				break;
		}

		// Backface culling state.
		GPU::setCullState(!material.twoSided(), Faces::BACK);

		// Bind the textures.
		program->textures(material.textures());
		GPU::drawMesh(*object.mesh());
	}

}

void DeferredRenderer::renderTransparent(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj){

	GPUMarker marker("Transparent objects");

	const auto & shadowMaps = _fwdLightsGPU->shadowMaps();

	GPU::setBlendState(true, BlendEquation::ADD, BlendFunction::ONE, BlendFunction::ONE_MINUS_SRC_ALPHA);
	GPU::setDepthState(true, TestFunction::LEQUAL, true);
	GPU::setCullState(true, Faces::BACK);


	// Update all shaders shared parameters.
	const glm::mat4 invView = glm::inverse(view);
	const glm::vec2 invScreenSize = 1.0f / glm::vec2(_lighting.width, _lighting.height);
	// Update shared data.
	Program* programs[] = {_transparentProgram, _transpIridProgram};
	for(Program* program : programs){
		program->uniform("inverseV", invView);
		program->uniform("lightsCount", int(_fwdLightsGPU->count()));
		program->uniform("probesCount", int(_fwdProbesGPU->count()));
		program->uniform("invScreenSize", invScreenSize);

		// This is because after a change of scene shadow maps and probes are reset, but the conditional setup of textures on
		// the program means that descriptors can still reference the deleted textures.
		program->defaultTexture(1);
		program->defaultTexture(2);
		program->defaultTexture(3);
	}

	for(const long & objectId : visibles) {
		// Once we get a -1, there is no other object to render.
		if(objectId == -1){
			break;
		}

		const auto & object = _scene->objects[objectId];
		const Material & material = object.material();
		// Skip non transparent objects.
		if((material.type() != Material::Type::Transparent) && (material.type() != Material::Type::TransparentIrid)){
			continue;
		}

		Program * currentProgram = nullptr;
		switch (material.type()) {
			case Material::Type::Transparent:
				currentProgram = _transparentProgram;
				break;
			case Material::Type::TransparentIrid:
				currentProgram = _transpIridProgram;
				break;
			default:
				break;
		}

		// Combine the three matrices.
		const glm::mat4 MV	= view * object.model();
		const glm::mat4 MVP = proj * MV;
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));

		// Upload the matrices.
		currentProgram->uniform("hasUV", object.useTexCoords());
		currentProgram->uniform("mvp", MVP);
		currentProgram->uniform("mv", MV);
		currentProgram->uniform("normalMatrix", glm::mat4(normalMatrix));

		// Bind the lights.
		currentProgram->buffer(_fwdLightsGPU->data(), 0);
		currentProgram->buffer(_fwdProbesGPU->data(), 1);
		currentProgram->bufferArray(_fwdProbesGPU->shCoeffs(), 2);
		
		// Bind the textures.
		currentProgram->texture(_textureBrdf, 0);
		currentProgram->textureArray(_fwdProbesGPU->envmaps(), 1);
		// Bind available shadow maps.
		if(shadowMaps[0]){
			currentProgram->texture(shadowMaps[0], 2);
		}
		if(shadowMaps[1]){
			currentProgram->texture(shadowMaps[1], 3);
		}
		// No SSAO as the objects are not rendered in it.

		// Objects textures.
		currentProgram->textures(material.textures(), 5);

		currentProgram->use();
		// To approximately handle two sided objects properly, draw the back faces first, then the front faces.
		// This won't solve all issues in case of concavities.
		if(material.twoSided()) {
			GPU::setCullState(true, Faces::FRONT);
			GPU::drawMesh(*object.mesh());
			GPU::setCullState(true, Faces::BACK);
		}
		GPU::drawMesh(*object.mesh());
	}
}

void DeferredRenderer::renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos){

	GPUMarker marker("Background");

	// Background.
	// No need to write the skybox depth to the framebuffer.
	// Accept a depth of 1.0 (far plane).
	GPU::setDepthState(true, TestFunction::LEQUAL, false);
	GPU::setBlendState(false);
	GPU::setCullState(false, Faces::BACK);

	const Object * background	= _scene->background.get();
	const Material& material = background->material();
	const Scene::Background mode = _scene->backgroundMode;
	
	if(mode == Scene::Background::SKYBOX) {
		// Skybox.
		const glm::mat4 backgroundMVP = proj * view * background->model();
		// Draw background.
		_skyboxProgram->use();
		// Upload the MVP matrix.
		_skyboxProgram->uniform("mvp", backgroundMVP);
		_skyboxProgram->textures(material.textures());
		GPU::drawMesh(*background->mesh());
		
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
		_atmoProgram->textures(material.textures());
		GPU::drawMesh(*background->mesh());
		
	} else {
		// Background color or 2D image.
		_bgProgram->use();
		if(mode == Scene::Background::IMAGE) {
			_bgProgram->uniform("useTexture", true);
			_bgProgram->textures(material.textures());
		} else {
			_bgProgram->uniform("useTexture", false);
			const glm::vec4& color = material.parameters()[0];
			_bgProgram->uniform("bgColor", glm::vec3(color));
		}
		GPU::drawMesh(*background->mesh());
	}
}

void DeferredRenderer::draw(const Camera & camera, Texture* dstColor, Texture* dstDepth, uint layer) {

	GPUMarker marker("Deferred render");

	assert(dstColor);
	assert(dstDepth == nullptr);

	const glm::mat4 & view = camera.view();
	const glm::mat4 & proj = camera.projection();
	const glm::vec3 & pos = camera.position();

	// Request list of visible objects from culler.
	const auto & visibles = _culler->cullAndSort(view, proj, pos);

	// Render opaque objects and the background to the Gbuffer.
	{
		GPUMarker marker("Gbuffer");
		// Clear the depth buffer (we know we will draw everywhere, no need to clear color).
		GPU::bind(Load::Operation::DONTCARE, 1.0f, Load::Operation::DONTCARE, &_sceneDepth, &_sceneAlbedo, &_sceneNormal, &_sceneEffects);
		GPU::setViewport(_sceneDepth);

		renderOpaque(visibles, view, proj);
		renderBackground(view, proj, pos);
	}

	// SSAO pass
	if(_applySSAO) {
		_ssaoPass->process(proj, _sceneDepth, _sceneNormal);
	} else {
		_ssaoPass->clear();
	}

	GPU::blitDepth(_sceneDepth, _depthCopy);

	// Gbuffer lighting pass
	_probeRenderer->updateCameraInfos(view, proj);
	_lightRenderer->updateCameraInfos(view, proj);

	// Accumulate probe contributions.
	{
		GPUMarker marker("Probes lighting");
		GPU::bind(glm::vec4(0.0f), &_indirectLighting);
		GPU::setViewport(_indirectLighting);
		for(const LightProbe& probe : _scene->probes){
			_probeRenderer->draw(probe);
		}
	}


	// Main lighting accumulation.
	{
		GPU::bind(Load::Operation::DONTCARE, Load::Operation::LOAD, Load::Operation::DONTCARE, &_depthCopy, &_lighting);
		GPU::setViewport(_lighting);

		// Merge probes contributions and background.
		{
			GPUMarker marker("Probes normalization");
			GPU::setDepthState(false);
			GPU::setBlendState(false);
			GPU::setCullState(true, Faces::BACK);
			_probeNormalization->use();
			_probeNormalization->texture(_sceneAlbedo, 0);
			_probeNormalization->texture(_sceneEffects, 1);
			_probeNormalization->texture(_indirectLighting, 2);
			GPU::drawQuad();
		}

		GPUMarker marker("Direct lighting");
		// Light contributions.
		for(auto & light : _scene->lights) {
			light->draw(*_lightRenderer);
		}
	}

	// If transparent objects are present, prepare the forward pass.
	if(_scene->transparent()){
		// Update forward light data.
		_fwdLightsGPU->updateCameraInfos(view, proj);
		
		for(const auto & light : _scene->lights) {
			light->draw(*_fwdLightsGPU);
		}
		_fwdLightsGPU->data().upload();
		// Update forward probes data.
		for(const auto & probe : _scene->probes) {
			_fwdProbesGPU->draw(probe);
		}
		_fwdProbesGPU->data().upload();
		// Now render transparent effects in a forward fashion.
		GPU::bind(Load::Operation::LOAD, Load::Operation::LOAD, Load::Operation::DONTCARE, &_depthCopy, &_lighting);
		GPU::setViewport(_lighting);
		renderTransparent(visibles, view, proj);
	}

	// Copy to the final texture.
	GPU::blit(_lighting, *dstColor, 0, layer, 0, 0, Filter::NEAREST);

}

void DeferredRenderer::resize(uint width, uint height) {
	// Resize the textures.
	const glm::vec2 nSize(width, height);
	_sceneAlbedo.resize(nSize);
	_sceneNormal.resize(nSize);
	_sceneEffects.resize(nSize);
	_sceneDepth.resize(nSize);
	_lighting.resize(nSize);
	_indirectLighting.resize(nSize);
	_depthCopy.resize(nSize);
	_ssaoPass->resize(width, height);
	
}

void DeferredRenderer::interface(){

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

Texture& DeferredRenderer::sceneDepth() {
	return _sceneDepth;
}
