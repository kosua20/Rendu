#include "StenciledRenderer.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/ScreenQuad.hpp"
#include "renderers/DebugViewer.hpp"


StenciledRenderer::StenciledRenderer(const glm::vec2 & resolution) {
	checkGLError();
}

void StenciledRenderer::setScene(const std::shared_ptr<Scene> & scene) {
	// Do not accept a null scene.
	if(!scene) {
		return;
	}
	_scene = scene;
	checkGLError();
}


void StenciledRenderer::draw(const Camera & camera, Framebuffer & framebuffer, size_t layer) {

	const glm::mat4 & view = camera.view();
	const glm::mat4 & proj = camera.projection();
}

void StenciledRenderer::resize(unsigned int width, unsigned int height) {
}
