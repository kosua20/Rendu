#include "EditorRenderer.hpp"
#include "input/Input.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/ScreenQuad.hpp"
#include "resources/Texture.hpp"
#include "scene/Sky.hpp"

EditorRenderer::EditorRenderer() :
	Renderer("Editor"), _lightsDebug("object_basic_uniform") {

	_preferredFormat.push_back({Layout::RGB8, Filter::LINEAR_NEAREST, Wrap::CLAMP});
	_needsDepth = true;
		
	_objectProgram	  = Resources::manager().getProgram("object_basic_lit_texture");
	_skyboxProgram	  = Resources::manager().getProgram("skybox_editor", "skybox_infinity", "skybox_basic");
	_bgProgram		  = Resources::manager().getProgram("background_infinity");
	_atmoProgram	  = Resources::manager().getProgram("atmosphere_editor", "background_infinity", "atmosphere_debug");
		
	Resources::manager().getTexture("debug-grid", { Layout::RGB8, Filter::LINEAR_LINEAR, Wrap::REPEAT}, Storage::GPU);
	
	checkGLError();
}

void EditorRenderer::setScene(const std::shared_ptr<Scene> & scene) {
	_scene = scene;
}

void EditorRenderer::draw(const Camera & camera, Framebuffer & framebuffer, size_t layer) {

	const glm::mat4 & view = camera.view();
	const glm::mat4 & proj = camera.projection();
	const glm::mat4 VP	   = proj * view;
	
	// Draw the scene.
	GLUtilities::setDepthState(true, TestFunction::LESS, true);
	GLUtilities::setCullState(false);
	framebuffer.bind(layer);
	framebuffer.setViewport();
	GLUtilities::clearColorAndDepth(glm::vec4(0.0f), 1.0f);
	
	// Render all objects.
	_objectProgram->use();
	for(auto & object : _scene->objects) {
		// Combine the three matrices.
		const glm::mat4 MVP			 = VP * object.model();
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(object.model())));
		_objectProgram->uniform("mvp", MVP);
		_objectProgram->uniform("normalMatrix", normalMatrix);
		
		const Texture * tex = Resources::manager().getTexture("debug-grid");
		if(!object.textures().empty()){
			tex = object.textures()[0];
		}
		GLUtilities::bindTexture(tex, 0);
		GLUtilities::drawMesh(*object.mesh());
	}
	
	// Render all lights.
	_lightsDebug.updateCameraInfos(view, proj);
	GLUtilities::setPolygonState(PolygonMode::LINE);
	for(auto & light : _scene->lights) {
		light->draw(_lightsDebug);
	}
	GLUtilities::setPolygonState(PolygonMode::FILL);
	
	// Render the background.
	renderBackground(view, proj, camera.position());
	
	GLUtilities::setDepthState(false);
	GLUtilities::setCullState(true, Faces::BACK);

}

void EditorRenderer::renderBackground(const glm::mat4 & view, const glm::mat4 & proj, const glm::vec3 & pos){
	// No need to write the skybox depth to the framebuffer.
	// Accept a depth of 1.0 (far plane).
	GLUtilities::setDepthState(true, TestFunction::LEQUAL, false);
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
		//GLUtilities::bindTextures(background->textures());
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
	GLUtilities::setDepthState(true, TestFunction::LESS, true);
}
