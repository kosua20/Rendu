#include "StenciledRenderer.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/ScreenQuad.hpp"
#include "renderers/DebugViewer.hpp"


StenciledRenderer::StenciledRenderer(const glm::vec2 & resolution) {

	const uint renderWidth	   = uint(resolution[0]);
	const uint renderHeight	   = uint(resolution[1]);

	// Framebuffer.
	const Descriptor descColor = {Layout::RGB8, Filter::LINEAR_LINEAR, Wrap::CLAMP};
	const Descriptor descDepth = {Layout::DEPTH24_STENCIL8, Filter::NEAREST_NEAREST, Wrap::CLAMP};
	const std::vector<Descriptor> descs = { descColor, descDepth};
	_sceneFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(renderWidth, renderHeight, descs, true, "Stenciled rendering"));

	_objectProgram	= Resources::manager().getProgram("object_basic_uniform", "object_basic", "object_basic_uniform");
	_fillProgram 	= Resources::manager().getProgram2D("fill-color");

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

	GLUtilities::setDepthState(false);
	GLUtilities::setCullState(true);

	_sceneFramebuffer->bind();
	_sceneFramebuffer->setViewport();

	_sceneFramebuffer->unbind();

	// Restore states.
	GLUtilities::setDepthState(false);
	GLUtilities::setCullState(true);
	// Output result.
	GLUtilities::blit(*_sceneFramebuffer, framebuffer, 0, layer, Filter::LINEAR);
}

void StenciledRenderer::resize(unsigned int width, unsigned int height) {
	// Resize the framebuffers.
	_sceneFramebuffer->resize(glm::vec2(width, height));
}
