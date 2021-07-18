#include "ForwardRenderer.hpp"
#include "scene/lights/Light.hpp"
#include "scene/Sky.hpp"
#include "system/System.hpp"
#include "graphics/GPU.hpp"
#include "graphics/ScreenQuad.hpp"

ForwardRenderer::ForwardRenderer(const glm::vec2 & resolution, ShadowMode mode, bool ssao, const std::string & name) :
	Renderer(name), _applySSAO(ssao), _shadowMode(mode) {

	const uint renderWidth	   = uint(resolution[0]);
	const uint renderHeight	   = uint(resolution[1]);

	// Framebuffers.
	const Descriptor desc = {Layout::RGBA16F, Filter::LINEAR_NEAREST, Wrap::CLAMP};
	const Descriptor descDepth = {Layout::DEPTH_COMPONENT32F, Filter::NEAREST_NEAREST, Wrap::CLAMP};
	const std::vector<Descriptor> descs = { desc, descDepth};
	_sceneFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, descs, true, _name + " Lighting "));
	_ssaoPass		  = std::unique_ptr<SSAO>(new SSAO(renderWidth, renderHeight, 2, 0.5f, _name));
	_preferredFormat.push_back({Layout::RGB16F, Filter::LINEAR_LINEAR, Wrap::CLAMP});
	_needsDepth = false;

	_depthPrepass 		= Resources::manager().getProgram("object_prepass_forward");
	_objectProgram		= Resources::manager().getProgram("object_forward");
	_parallaxProgram	= Resources::manager().getProgram("object_parallax_forward");
	_emissiveProgram	= Resources::manager().getProgram("object_emissive_forward");
	_transparentProgram = Resources::manager().getProgram("object_transparent_forward", "object_forward", "object_transparent_forward");

	_skyboxProgram = Resources::manager().getProgram("skybox_forward", "skybox_infinity", "skybox_forward");
	_bgProgram	   = Resources::manager().getProgram("background_forward", "background_infinity", "background_forward");
	_atmoProgram   = Resources::manager().getProgram("atmosphere_forward", "background_infinity", "atmosphere_forward");

	_textureBrdf = Resources::manager().getTexture("brdf-precomputed", {Layout::RG32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::GPU);

	checkGPUError();
}

void ForwardRenderer::setScene(const std::shared_ptr<Scene> & scene) {
	// Do not accept a null scene.
	if(!scene) {
		return;
	}
	_scene = scene;
	_culler.reset(new Culler(_scene->objects));
	_lightsGPU.reset(new ForwardLight(_scene->lights.size()));
	checkGPUError();
}

void ForwardRenderer::renderDepth(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj){

	GPU::setDepthState(true, TestFunction::LESS, true);
	GPU::setCullState(true, Faces::BACK);
	GPU::setBlendState(false);

	_sceneFramebuffer->bind({0.5f,0.5f,0.5f,1.0f}, 1.0f);
	_sceneFramebuffer->setViewport();
	// We use the depth prepass to store packed normals in the color target.
	// We initialize using null normal.

	_depthPrepass->use();
	for(const long & objectId : visibles) {
		// Once we get a -1, there is no other object to render.
		if(objectId == -1){
			break;
		}

		const auto & object = _scene->objects[objectId];

		// Render using prepass shader.
		// We skip parallax mapped objects as their depth is going to change.
		// We also skip all transparent/refractive objects
		if(object.type() == Object::Parallax || object.type() == Object::Transparent){
			continue;
		}

		// Upload the matrices.
		const glm::mat4 MV	= view * object.model();
		const glm::mat4 MVP = proj * MV;
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));

		_depthPrepass->uniform("mvp", MVP);
		_depthPrepass->uniform("normalMatrix", normalMatrix);
		// Alpha mask if needed.
		_depthPrepass->uniform("hasMask", object.masked());
		_depthPrepass->uniform("hasUV", object.useTexCoords());
		
		if(object.masked()) {
			_depthPrepass->texture(object.textures()[0], 0);
		}
		// Backface culling state.
		GPU::setCullState(!object.twoSided(), Faces::BACK);
		GPU::drawMesh(*object.mesh());
	}
}

void ForwardRenderer::renderOpaque(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj){

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
		// Skip transparent objects.
		if(object.type() == Object::Type::Transparent){
			continue;
		}

		// Combine the three matrices.
		const glm::mat4 MV	= view * object.model();
		const glm::mat4 MVP = proj * MV;

		// Shortcut for emissive objects as their shader is quite different from other PBR shaders.
		if(object.type() == Object::Type::Emissive){
			_emissiveProgram->use();
			_emissiveProgram->uniform("mvp", MVP);
			_emissiveProgram->uniform("hasUV", object.useTexCoords());
			if(object.twoSided()) {
				GPU::setCullState(false);
			}
			// Bind the textures.
			_emissiveProgram->textures(object.textures());
			GPU::drawMesh(*object.mesh());
			GPU::setCullState(true, Faces::BACK);
			continue;
		}

		// Select the program (and shaders).
		Program * currentProgram = nullptr;
		switch(object.type()) {
			case Object::Parallax:
				currentProgram = _parallaxProgram;
				break;
			case Object::Regular:
				currentProgram = _objectProgram;
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
		currentProgram->uniform("normalMatrix", normalMatrix);

		// Backface culling state.
		GPU::setCullState(!object.twoSided(), Faces::BACK);
		// Bind the lights.
		currentProgram->buffer(_lightsGPU->data(), 0);
		currentProgram->buffer(*_scene->environment.shCoeffs(), 1);
		// Bind the textures.
		currentProgram->textures(object.textures());
		currentProgram->texture(_textureBrdf, 4);
		currentProgram->texture(_scene->environment.map(), 5);
		// Bind available shadow maps.
		if(shadowMaps[0]){
			currentProgram->texture(shadowMaps[0], 6);
		}
		if(shadowMaps[1]){
			currentProgram->texture(shadowMaps[1], 7);
		}
		currentProgram->texture(_ssaoPass->texture(), 8);
		GPU::drawMesh(*object.mesh());
	}

}

void ForwardRenderer::renderTransparent(const Culler::List & visibles, const glm::mat4 & view, const glm::mat4 & proj){

	const auto & shadowMaps = _lightsGPU->shadowMaps();

	GPU::setBlendState(true, BlendEquation::ADD, BlendFunction::ONE, BlendFunction::ONE_MINUS_SRC_ALPHA);
	GPU::setDepthState(true, TestFunction::LEQUAL, true);
	GPU::setCullState(true, Faces::BACK);

	_transparentProgram->use();
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
		_transparentProgram->buffer(_lightsGPU->data(), 0);
		_transparentProgram->buffer(*_scene->environment.shCoeffs(), 1);
		// Bind the textures.
		_transparentProgram->textures(object.textures());
		_transparentProgram->texture(_textureBrdf, 4);
		_transparentProgram->texture(_scene->environment.map(), 5);
		// Bind available shadow maps.
		if(shadowMaps[0]){
			_transparentProgram->texture(shadowMaps[0], 6);
		}
		if(shadowMaps[1]){
			_transparentProgram->texture(shadowMaps[1], 7);
		}
		// No SSAO as the objects are not rendered in it.

		// To approximately handle two sided objects properly, draw the back faces first, then the front faces.
		// This won't solve all issues in case of concavities.
		if(object.twoSided()) {
			GPU::setCullState(true, Faces::FRONT);
			GPU::drawMesh(*object.mesh());
			GPU::setCullState(true, Faces::BACK);
		}
		GPU::drawMesh(*object.mesh());
	}
}

void ForwardRenderer::renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos) {
	// No need to write the skybox depth to the framebuffer.
	// Accept a depth of 1.0 (far plane).
	GPU::setDepthState(true, TestFunction::LEQUAL, false);
	GPU::setBlendState(false);
	GPU::setCullState(true, Faces::BACK);
	const Object * background	 = _scene->background.get();
	const Scene::Background mode = _scene->backgroundMode;

	if(mode == Scene::Background::SKYBOX) {
		// Skybox.
		const glm::mat4 backgroundMVP = proj * view * background->model();
		// Draw background.
		_skyboxProgram->use();
		// Upload the MVP matrix.
		_skyboxProgram->uniform("mvp", backgroundMVP);
		_skyboxProgram->textures(background->textures());
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
		_atmoProgram->textures(background->textures());
		GPU::drawMesh(*background->mesh());

	} else {
		// Background color or 2D image.
		_bgProgram->use();
		if(mode == Scene::Background::IMAGE) {
			_bgProgram->uniform("useTexture", 1);
			_bgProgram->textures(background->textures());
		} else {
			_bgProgram->uniform("useTexture", 0);
			_bgProgram->uniform("bgColor", _scene->backgroundColor);
		}
		GPU::drawMesh(*background->mesh());
	}

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

	// Select visible objects.
	const auto & visibles = _culler->cullAndSort(view, proj, pos);

	// Depth and normas prepass.
	renderDepth(visibles, view, proj);

	// SSAO pass
	if(_applySSAO) {
		_ssaoPass->process(proj, _sceneFramebuffer->depthBuffer(), _sceneFramebuffer->texture());
	} else {
		_ssaoPass->clear();
	}

	// Update all shaders shared parameters.
	{
		const LightProbe & environment = _scene->environment;
		const float cubeLod		= float(environment.map()->levels - 1);
		const glm::mat4 invView = glm::inverse(view);
		const glm::vec2 invScreenSize = 1.0f / glm::vec2(_sceneFramebuffer->width(), _sceneFramebuffer->height());
		// Update shared data for the three programs.
		Program * programs[] = {_parallaxProgram, _objectProgram, _transparentProgram };
		for(Program * prog : programs){
			prog->use();
			prog->uniform("inverseV", invView);
			prog->uniform("maxLod", cubeLod);
			prog->uniform("cubemapPos", environment.position());
			prog->uniform("cubemapCenter", environment.center());
			prog->uniform("cubemapExtent", environment.extent());
			prog->uniform("cubemapCosSin", environment.rotationCosSin());
			prog->uniform("lightsCount", int(_lightsGPU->count()));
			prog->uniform("invScreenSize", invScreenSize);
		}
		_parallaxProgram->use();
		_parallaxProgram->uniform("p", proj);
	}

	// Objects rendering.
	_sceneFramebuffer->bind(glm::vec4(0.0f), 1.0f);
	_sceneFramebuffer->setViewport();
	// Render opaque objects.
	renderOpaque(visibles, view, proj);
	// Render the backgound.
	renderBackground(view, proj, pos);
	// Render transparent objects.
	renderTransparent(visibles, view, proj);

	// Final composite pass
	GPU::blit(*_sceneFramebuffer, framebuffer, 0, layer, Filter::LINEAR);
}

void ForwardRenderer::resize(unsigned int width, unsigned int height) {
	// Resize the framebuffers.
	_ssaoPass->resize(width / 2, height / 2);
	_sceneFramebuffer->resize(glm::vec2(width, height));
	checkGPUError();
}

void ForwardRenderer::interface(){

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

const Framebuffer * ForwardRenderer::sceneDepth() const {
	return _sceneFramebuffer.get();
}

const Texture * ForwardRenderer::sceneNormal() const {
	return _sceneFramebuffer->texture(2);
}
