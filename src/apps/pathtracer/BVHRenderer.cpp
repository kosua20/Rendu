#include "BVHRenderer.hpp"
#include "input/Input.hpp"
#include "system/System.hpp"
#include "graphics/GLUtilities.hpp"
#include "graphics/ScreenQuad.hpp"
#include "resources/Texture.hpp"

BVHRenderer::BVHRenderer(const glm::vec2 & resolution) {
	// Setup camera parameters.
	_renderResolution = resolution;
	// GL setup
	_sceneFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(uint(_renderResolution[0]), uint(_renderResolution[1]), {Layout::RGB8, Filter::LINEAR_NEAREST, Wrap::CLAMP}, true));
	_objectProgram	= Resources::manager().getProgram("object_basic_lit");
	_bvhProgram		  = Resources::manager().getProgram("object_basic_color");
	_renderResult = _sceneFramebuffer->textureId();
	checkGLError();
}

void BVHRenderer::setScene(const std::shared_ptr<Scene> & scene, const Raycaster & raycaster) {
	_scene = scene;
	_visuHelper = std::unique_ptr<RaycasterVisualisation>(new RaycasterVisualisation(raycaster));
	// Build the BVH mesh.
	_visuHelper->getAllLevels(_bvhLevels);
	for(Mesh & level : _bvhLevels) {
		// Setup the OpenGL mesh, don't keep the CPU mesh.
		level.upload();
		level.clearGeometry();
	}
	_bvhRange = glm::vec2(0, 0);
	checkGLError();
}

void BVHRenderer::draw(const Camera & camera) {

	// Draw the scene.
	glEnable(GL_DEPTH_TEST);
	_sceneFramebuffer->bind();
	_sceneFramebuffer->setViewport();
	GLUtilities::clearColorAndDepth(glm::vec4(0.0f), 1.0f);
	glDisable(GL_CULL_FACE);

	const glm::mat4 & view = camera.view();
	const glm::mat4 & proj = camera.projection();
	const glm::mat4 VP	 = proj * view;

	_objectProgram->use();
	for(auto & object : _scene->objects) {
		// Combine the three matrices.
		const glm::mat4 MVP			 = VP * object.model();
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(object.model())));
		_objectProgram->uniform("mvp", MVP);
		_objectProgram->uniform("normalMatrix", normalMatrix);
		GLUtilities::drawMesh(*object.mesh());
	}

	// Debug wireframe visualisation.
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	_bvhProgram->use();
	_bvhProgram->uniform("mvp", VP);
	// If there is a ray mesh, show it.
	if(_rayVis.gpu && _rayVis.gpu->count > 0) {
		GLUtilities::drawMesh(_rayVis);
		if(_showBVH) {
			for(int lid = _bvhRange.x; lid <= _bvhRange.y; ++lid) {
				if(lid >= int(_rayLevels.size())) {
					break;
				}
				GLUtilities::drawMesh(_rayLevels[lid]);
			}
		}
	} else if(_showBVH) {
		for(int lid = _bvhRange.x; lid <= _bvhRange.y; ++lid) {
			GLUtilities::drawMesh(_bvhLevels[lid]);
		}
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	_sceneFramebuffer->unbind();
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

void BVHRenderer::clean() {
	_sceneFramebuffer->clean();
	for(Mesh & level : _bvhLevels) {
		level.clean();
	}
	for(Mesh & level : _rayLevels) {
		level.clean();
	}
	_rayVis.clean();
}

void BVHRenderer::resize(unsigned int width, unsigned int height) {
	_renderResolution = glm::vec2(width, height);
	// Resize the framebuffers.
	_sceneFramebuffer->resize(_renderResolution);
}

void BVHRenderer::castRay(const glm::vec3 & position, const glm::vec3 & direction) {

	// Intersect.
	const Raycaster::Hit hit = _visuHelper->getRayLevels(position, direction, _rayLevels);
	// Level meshes.
	for(Mesh & level : _rayLevels) {
		// Setup the OpenGL mesh, don't keep the CPU mesh.
		level.upload();
		level.clearGeometry();
	}
	// Ray and intersection mesh.
	const float defaultLength = 3.0f * glm::length(_scene->boundingBox().getSize());

	_visuHelper->getRayMesh(position, direction, hit, _rayVis, defaultLength);
	_rayVis.upload();
	_rayVis.clearGeometry();
}

void BVHRenderer::clearRay(){
	_rayVis.clean();
	for(auto & level : _rayLevels) {
		level.clean();
	}
	_rayLevels.clear();
}

int BVHRenderer::maxLevel(){
	return int(_bvhLevels.size()) - 1;
}
