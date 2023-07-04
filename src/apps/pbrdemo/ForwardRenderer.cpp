#include "ForwardRenderer.hpp"
#include "scene/lights/Light.hpp"
#include "scene/Sky.hpp"
#include "system/System.hpp"
#include "graphics/GPU.hpp"


ForwardRenderer::ForwardRenderer(const glm::vec2 & resolution, bool ssao, const std::string & name) :
	Renderer(name), _sceneColor(name + "Lighting"), _sceneDepth(name + "Depth"), _applySSAO(ssao) {

	const uint renderWidth	   = uint(resolution[0]);
	const uint renderHeight	   = uint(resolution[1]);

	// Attachments.
	_sceneColor.setupAsDrawable(Layout::RGBA16F, renderWidth, renderHeight);
	_sceneDepth.setupAsDrawable(Layout::DEPTH_COMPONENT32F, renderWidth, renderHeight);
	_ssaoPass		  = std::unique_ptr<SSAO>(new SSAO(renderWidth, renderHeight, 2, 0.5f, _name));
	_colorFormat 	  = Layout::RGBA16F;

	_depthPrepass 		= Resources::manager().getProgram("object_prepass_forward");
	_objectProgram		= Resources::manager().getProgram("object_forward");
	_parallaxProgram	= Resources::manager().getProgram("object_parallax_forward");
	_emissiveProgram	= Resources::manager().getProgram("object_emissive_forward", "object_forward", "object_emissive_forward");
	_transparentProgram = Resources::manager().getProgram("object_transparent_forward", "object_forward", "object_transparent_forward");
	_transpIridProgram  = Resources::manager().getProgram("object_transparent_irid_forward", "object_forward", "object_transparent_irid_forward");
	_clearCoatProgram 	= Resources::manager().getProgram("object_clearcoat_forward", "object_forward", "object_clearcoat_forward");
	_anisotropicProgram = Resources::manager().getProgram("object_anisotropic_forward", "object_forward", "object_anisotropic_forward");
	_sheenProgram  		= Resources::manager().getProgram("object_sheen_forward", "object_forward", "object_sheen_forward");
	_iridescentProgram  = Resources::manager().getProgram("object_iridescent_forward", "object_forward", "object_iridescent_forward");
	_subsurfaceProgram  = Resources::manager().getProgram("object_subsurface_forward", "object_forward", "object_subsurface_forward");
	_skyboxProgram = Resources::manager().getProgram("skybox_forward", "skybox_infinity", "skybox_forward");
	_bgProgram	   = Resources::manager().getProgram("background_forward", "background_infinity", "background_forward");
	_atmoProgram   = Resources::manager().getProgram("atmosphere_forward", "background_infinity", "atmosphere_forward");

	_textureBrdf = Resources::manager().getTexture("brdf-precomputed", Layout::RGBA16F, Storage::GPU);

}

void ForwardRenderer::setScene(const std::shared_ptr<Scene> & scene) {
	// Do not accept a null scene.
	if(!scene) {
		return;
	}
	_scene = scene;
	_culler.reset(new Culler(_scene->objects));
	_lightsGPU.reset(new ForwardLight(_scene->lights.size()));
	_probesGPU.reset(new ForwardProbe(_scene->probes.size()));
}

void ForwardRenderer::renderDepth(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj){

	GPUMarker marker("Z prepass");

	GPU::setDepthState(true, TestFunction::LESS, true);
	GPU::setCullState(true, Faces::BACK);
	GPU::setBlendState(false);

	GPU::bind(glm::vec4(0.5f,0.5f,0.5f,1.0f), 1.0f, Load::Operation::DONTCARE, &_sceneDepth, &_sceneColor);
	GPU::setViewport(_sceneColor);

	// We use the depth prepass to store packed normals in the color target.
	// We initialize using null normal.

	_depthPrepass->use();
	_depthPrepass->defaultTexture(0);
	
	for(const long & objectId : visibles) {
		// Once we get a -1, there is no other object to render.
		if(objectId == -1){
			break;
		}

		const Object & object = _scene->objects[objectId];
		const Material & material = object.material();
		// Render using prepass shader.
		// We skip parallax mapped objects as their depth is going to change.
		// We also skip all transparent/refractive objects
		if(material.type() == Material::Parallax || material.type() == Material::Transparent || material.type() == Material::Type::TransparentIrid){
			continue;
		}

		// Upload the matrices.
		const glm::mat4 MV	= view * object.model();
		const glm::mat4 MVP = proj * MV;
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));

		_depthPrepass->uniform("mvp", MVP);
		_depthPrepass->uniform("normalMatrix", glm::mat4(normalMatrix));
		// Alpha mask if needed.
		_depthPrepass->uniform("hasMask", material.masked());
		_depthPrepass->uniform("hasUV", object.useTexCoords());
		
		if(material.masked()) {
			_depthPrepass->texture(material.textures()[0], 0);
		}
		// Backface culling state.
		GPU::setCullState(!material.twoSided(), Faces::BACK);
		GPU::drawMesh(*object.mesh());
	}
}

void ForwardRenderer::renderOpaque(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj){

	GPUMarker marker("Opaque objects");

	GPU::setDepthState(true, TestFunction::LEQUAL, true);
	GPU::setCullState(true, Faces::BACK);
	GPU::setBlendState(false);

	const auto & shadowMaps = _lightsGPU->shadowMaps();

	// Scene objects.
	for(const long & objectId : visibles) {
		// Once we get a -1, there is no other object to render.
		if(objectId == -1){
			break;
		}

		const auto & object = _scene->objects[objectId];
		const Material & material = object.material();
		// Skip transparent objects.
		if((material.type() == Material::Type::Transparent) || (material.type() == Material::Type::TransparentIrid)){
			continue;
		}

		// Combine the three matrices.
		const glm::mat4 MV	= view * object.model();
		const glm::mat4 MVP = proj * MV;

		// Select the program (and shaders).
		Program * currentProgram = nullptr;
		switch(material.type()) {
			case Material::Parallax:
				currentProgram = _parallaxProgram;
				break;
			case Material::Regular:
				currentProgram = _objectProgram;
				break;
			case Material::Clearcoat:
				currentProgram = _clearCoatProgram;
				break;
			case Material::Anisotropic:
				currentProgram = _anisotropicProgram;
				break;
			case Material::Sheen:
				currentProgram = _sheenProgram;
				break;
			case Material::Iridescent:
				currentProgram = _iridescentProgram;
				break;
			case Material::Subsurface:
				currentProgram = _subsurfaceProgram;
				break;
			case Material::Emissive:
				currentProgram = _emissiveProgram;
				break;
			default:
				Log::Error() << "Unsupported material type." << std::endl;
				continue;
				break;
		}
		// Compute the normal matrix
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));

		// Upload the matrices.
		currentProgram->use();
		currentProgram->uniform("hasUV", object.useTexCoords());
		currentProgram->uniform("mvp", MVP);
		currentProgram->uniform("mv", MV);
		currentProgram->uniform("normalMatrix", glm::mat4(normalMatrix));

		// Backface culling state.
		GPU::setCullState(!material.twoSided(), Faces::BACK);
		// Bind the lights.
		currentProgram->buffer(_lightsGPU->data(), 0);
		currentProgram->buffer(_probesGPU->data(), 1);
		currentProgram->bufferArray(_probesGPU->shCoeffs(), 2);
		// Bind the textures.
		currentProgram->texture(_textureBrdf, 0);
		currentProgram->textureArray(_probesGPU->envmaps(), 1);
		// Bind available shadow maps.
		if(shadowMaps[0]){
			currentProgram->texture(shadowMaps[0], 2);
		}
		if(shadowMaps[1]){
			currentProgram->texture(shadowMaps[1], 3);
		}
		currentProgram->texture(_ssaoPass->texture(), 4);
		currentProgram->textures(material.textures(), 5);
		
		GPU::drawMesh(*object.mesh());
	}

}

void ForwardRenderer::renderTransparent(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj){

	GPUMarker marker("Transparent objects");

	const auto & shadowMaps = _lightsGPU->shadowMaps();

	GPU::setBlendState(true, BlendEquation::ADD, BlendFunction::ONE, BlendFunction::ONE_MINUS_SRC_ALPHA);
	GPU::setDepthState(true, TestFunction::LEQUAL, true);
	GPU::setCullState(true, Faces::BACK);


	for(const long & objectId : visibles) {
		// Once we get a -1, there is no other object to render.
		if(objectId == -1){
			break;
		}

		const auto & object = _scene->objects[objectId];
		const Material& material = object.material();
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

		// Bind the lights and probes.
		currentProgram->buffer(_lightsGPU->data(), 0);
		currentProgram->buffer(_probesGPU->data(), 1);
		currentProgram->bufferArray(_probesGPU->shCoeffs(), 2);
		// Bind the textures.
		currentProgram->texture(_textureBrdf, 0);
		currentProgram->textureArray(_probesGPU->envmaps(), 1);
		// Bind available shadow maps.
		if(shadowMaps[0]){
			currentProgram->texture(shadowMaps[0], 2);
		}
		if(shadowMaps[1]){
			currentProgram->texture(shadowMaps[1], 3);
		}
		// No SSAO as the objects are not rendered in it.

		// Material textures.
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

void ForwardRenderer::renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos) {
	GPUMarker marker("Background");

	// No need to write the skybox depth to the framebuffer.
	// Accept a depth of 1.0 (far plane).
	GPU::setDepthState(true, TestFunction::LEQUAL, false);
	GPU::setBlendState(false);
	GPU::setCullState(false, Faces::BACK);
	const Object * background	 = _scene->background.get();
	const Material & material	 = background->material();
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

void ForwardRenderer::draw(const Camera & camera, Texture* dstColor, Texture* dstDepth, uint layer) {
	assert(dstColor);
	assert(dstDepth == nullptr);

	GPUMarker marker("Forward render");

	const glm::mat4 & view = camera.view();
	const glm::mat4 & proj = camera.projection();
	const glm::vec3 & pos  = camera.position();
	// --- Update lights data ----
	_lightsGPU->updateCameraInfos(view, proj);

	for(const auto & light : _scene->lights) {
		light->draw(*_lightsGPU);
	}
	_lightsGPU->data().upload();

	// --- Update probes data
	for(const auto& probe : _scene->probes){
		_probesGPU->draw(probe);
	}
	_probesGPU->data().upload();

	// Select visible objects.
	const auto & visibles = _culler->cullAndSort(view, proj, pos);

	// Depth and normas prepass.
	renderDepth(visibles, view, proj);

	// SSAO pass
	if(_applySSAO) {
		_ssaoPass->process(proj, _sceneDepth, _sceneColor);
	} else {
		_ssaoPass->clear();
	}

	// Update all shaders shared parameters.
	{
		const glm::mat4 invView = glm::inverse(view);
		const glm::vec2 invScreenSize = 1.0f / glm::vec2(_sceneColor.width, _sceneColor.height);
		// Update shared data for the three programs.
		Program * programs[] = {_parallaxProgram, _objectProgram, _clearCoatProgram, _transparentProgram, _transpIridProgram, _emissiveProgram, _anisotropicProgram, _sheenProgram, _iridescentProgram, _subsurfaceProgram };
		for(Program * prog : programs){
			prog->use();
			prog->uniform("inverseV", invView);
			prog->uniform("probesCount", int(_probesGPU->count()));
			prog->uniform("lightsCount", int(_lightsGPU->count()));
			prog->uniform("invScreenSize", invScreenSize);
			/// This is because after a change of scene shadow maps and probes are reset, but the conditional setup of textures on
			/// the program means that descriptors can still reference the deleted textures.
			/// \todo Currently there is no mechanism to "unregister" a texture for each shader using it, when deleting the texture.
			/// The texture could keep a record of all programs it has been used in. Or we could look at all programs when deleting.
			/// Or in PBRDemo we reset the textures when setting a scene.
			prog->defaultTexture(1);
			prog->defaultTexture(2);
			prog->defaultTexture(3);
		}
		_parallaxProgram->use();
		_parallaxProgram->uniform("p", proj);
	}

	// Objects rendering.
	GPU::bind(glm::vec4(0.0f), 1.0f, Load::Operation::DONTCARE, &_sceneDepth, &_sceneColor);
	GPU::setViewport(_sceneColor);
	// Render opaque objects.
	renderOpaque(visibles, view, proj);
	// Render the backgound.
	renderBackground(view, proj, pos);
	// Render transparent objects.
	renderTransparent(visibles, view, proj);

	// Final composite pass
	GPU::blit(_sceneColor, *dstColor, 0, layer, Filter::LINEAR);
}

void ForwardRenderer::resize(uint width, uint height) {
	// Resize the textures.
	_ssaoPass->resize(width / 2, height / 2);
	_sceneColor.resize(width, height);
	_sceneDepth.resize(width, height);
}

void ForwardRenderer::interface(){

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

Texture& ForwardRenderer::sceneDepth() {
	return _sceneDepth;
}
