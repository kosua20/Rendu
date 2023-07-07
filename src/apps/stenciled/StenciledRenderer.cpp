#include "StenciledRenderer.hpp"
#include "system/System.hpp"
#include "graphics/GPU.hpp"
#include "renderers/DebugViewer.hpp"


StenciledRenderer::StenciledRenderer(const glm::vec2 & resolution) : Renderer("Stenciled"), _sceneColor("Stenciled color"), _sceneDepth("Stenciled depth") {

	const uint renderWidth	   = uint(resolution[0]);
	const uint renderHeight	   = uint(resolution[1]);

	// Attahcments.
	_sceneColor.setupAsDrawable(Layout::RGBA8, renderWidth, renderHeight);
	_sceneDepth.setupAsDrawable(Layout::DEPTH32F_STENCIL8, renderWidth, renderHeight);

	_objectProgram	= Resources::manager().getProgram("object_basic_uniform", "object_basic", "object_basic_uniform");
	_fillProgram 	= Resources::manager().getProgram2D("fill-color");

}

void StenciledRenderer::setScene(const std::shared_ptr<Scene> & scene) {
	// Do not accept a null scene.
	if(!scene) {
		return;
	}
	_scene = scene;
}


void StenciledRenderer::draw(const Camera & camera, Texture* dstColor, Texture* dstDepth, uint layer) {
	assert(dstColor);
	assert(dstDepth == nullptr);

	const glm::mat4 & view = camera.view();
	const glm::mat4 & proj = camera.projection();

	GPU::setDepthState(false);
	GPU::setCullState(true, Faces::BACK);
	GPU::setBlendState(false);

	GPU::beginRender(1.0f, (uchar)0x0, &_sceneDepth, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), &_sceneColor);
	GPU::setViewport(_sceneDepth);

	// Clear colorbuffer to white, don't write to it for now.
	GPU::setColorState(false, false, false, false);
	// Always pass stencil test and flip all bits. As triangles are rendered successively to a pixel,
	// they will flip the value between 0x00 (even count) and 0xFF (odd count).
	GPU::setStencilState(true, TestFunction::ALWAYS, StencilOp::KEEP, StencilOp::INVERT, StencilOp::INVERT, 0x00);

	DebugViewer::trackStateDefault("Object");

	// Scene objects.
	// Render all objects with a simple program.
	_objectProgram->use();
	_objectProgram->uniform("color", glm::vec4(1.0f));
	const glm::mat4 VP = proj*view;
	const Frustum camFrustum(VP);
	for(auto & object : _scene->objects) {
		// Check visibility.
		if(!camFrustum.intersects(object.boundingBox())){
			continue;
		}
		// Combine the three matrices.
		const glm::mat4 MVP = VP * object.model();

		// Upload the matrices.
		_objectProgram->uniform("mvp", MVP);
		
		// Backface culling state.
		GPU::setCullState(!object.material().twoSided(), Faces::BACK);
		GPU::drawMesh(*object.mesh());
	}

	// Render a black quad only where the stencil buffer is non zero (ie odd count of covering primitives).
	GPU::setStencilState(true, TestFunction::NOTEQUAL, StencilOp::KEEP, StencilOp::KEEP, StencilOp::KEEP, 0x00);
	GPU::setColorState(true, true, true, true);
	GPU::setCullState(true, Faces::BACK);
	
	DebugViewer::trackStateDefault("Screen");

	_fillProgram->use();
	_fillProgram->uniform("color", glm::vec4(0.0f));
	GPU::drawQuad();
	GPU::endRender();

	// Restore stencil state.
	GPU::setStencilState(false, false);

	DebugViewer::trackStateDefault("Off stencil");

	// Output result.
	GPU::blit(_sceneColor, *dstColor, 0, layer, Filter::LINEAR);
}

void StenciledRenderer::resize(uint width, uint height) {
	// Resize the textures.
	_sceneColor.resize(width, height);
	_sceneDepth.resize(width, height);
}
