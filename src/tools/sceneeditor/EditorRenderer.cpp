#include "EditorRenderer.hpp"
#include "input/Input.hpp"
#include "system/System.hpp"
#include "graphics/GPU.hpp"
#include "resources/Texture.hpp"
#include "scene/Sky.hpp"

EditorRenderer::EditorRenderer() :
	Renderer("Editor"), _lightsDebug("object_basic_uniform") {

	_colorFormat = Layout::RGBA8;
	_depthFormat = Layout::DEPTH_COMPONENT32F;
		
	_objectProgram	  = Resources::manager().getProgram("object_basic_lit_texture");
	_skyboxProgram	  = Resources::manager().getProgram("skybox_editor", "skybox_infinity", "skybox_basic");
	_bgProgram		  = Resources::manager().getProgram("background_infinity");
	_atmoProgram	  = Resources::manager().getProgram("atmosphere_editor", "background_infinity", "atmosphere_debug");
		
	Resources::manager().getTexture("debug-grid", Layout::RGBA8, Storage::GPU);
	
}

void EditorRenderer::setScene(const std::shared_ptr<Scene> & scene) {
	_scene = scene;
}

void EditorRenderer::draw(const Camera & camera, Texture* dstColor, Texture* dstDepth, uint layer) {
	assert(dstColor);
	assert(dstDepth);

	const glm::mat4 & view = camera.view();
	const glm::mat4 & proj = camera.projection();
	const glm::mat4 VP	   = proj * view;
	
	// Draw the scene.
	GPU::setDepthState(true, TestFunction::LESS, true);
	GPU::setCullState(false);
	GPU::beginRender(layer, 0, 1.0f, Load::Operation::DONTCARE, dstDepth, glm::vec4(0.0f), dstColor);
	GPU::setViewport(*dstColor);
	
	// Render all objects.
	_objectProgram->use();
	_objectProgram->uniform("lightDir", glm::vec3(0.577f));
	for(auto & object : _scene->objects) {
		// Combine the three matrices.
		const glm::mat4 MVP			 = VP * object.model();
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(object.model())));
		_objectProgram->uniform("mvp", MVP);
		_objectProgram->uniform("normalMatrix", glm::mat4(normalMatrix));
		
		const Texture * tex = Resources::manager().getTexture("debug-grid");
		if(!object.material().textures().empty()){
			tex = object.material().textures()[0];
		}
		_objectProgram->texture(tex, 0);
		GPU::drawMesh(*object.mesh());
	}
	
	// Render all lights.
	_lightsDebug.updateCameraInfos(view, proj);
	GPU::setPolygonState(PolygonMode::LINE);
	for(auto & light : _scene->lights) {
		light->draw(_lightsDebug);
	}
	GPU::setPolygonState(PolygonMode::FILL);
	
	// Render the background.
	renderBackground(view, proj, camera.position());
	
	GPU::setDepthState(false);
	GPU::setCullState(true, Faces::BACK);
	GPU::endRender();

}

void EditorRenderer::renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos){
	// No need to write the skybox depth to the framebuffer.
	// Accept a depth of 1.0 (far plane).
	GPU::setDepthState(true, TestFunction::LEQUAL, false);
	const Object * background	= _scene->background.get();
	const Scene::Background mode = _scene->backgroundMode;
	
	if(mode == Scene::Background::SKYBOX) {
		// Skybox.
		const glm::mat4 backgroundMVP = proj * view * background->model();
		// Draw background.
		_skyboxProgram->use();
		// Upload the MVP matrix.
		_skyboxProgram->uniform("mvp", backgroundMVP);
		_skyboxProgram->textures(background->material().textures());
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
		//GPU::bindTextures(background->textures());
		GPU::drawMesh(*background->mesh());
		
	} else {
		// Background color or 2D image.
		_bgProgram->use();
		if(mode == Scene::Background::IMAGE) {
			_bgProgram->uniform("useTexture", 1);
			_bgProgram->textures(background->material().textures());
		} else {
			_bgProgram->uniform("useTexture", 0);
			const glm::vec4 & color = background->material().parameters()[0];
			_bgProgram->uniform("bgColor", glm::vec3(color));
		}
		GPU::drawMesh(*background->mesh());
	}
	GPU::setDepthState(true, TestFunction::LESS, true);
}
