#include "BVHRenderer.hpp"
#include "input/Input.hpp"
#include "system/System.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/ScreenQuad.hpp"
#include "resources/Texture.hpp"

BVHRenderer::BVHRenderer() : Renderer("BVH renderer") {
	// GL setup
	_preferredFormat.push_back({Layout::RGBA8, Filter::LINEAR_NEAREST, Wrap::CLAMP});
	_needsDepth = true;
	_objectProgram = Resources::manager().getProgram("object_basic_lit");
	_bvhProgram = Resources::manager().getProgram("object_basic_color");
	checkGPUError();
}

void BVHRenderer::setScene(const std::shared_ptr<Scene> & scene, const Raycaster & raycaster) {
	_scene = scene;
	_visuHelper = std::unique_ptr<RaycasterVisualisation>(new RaycasterVisualisation(raycaster));
	// Build the BVH mesh.
	_visuHelper->getAllLevels(_bvhLevels);
	for(Mesh & level : _bvhLevels) {
		// Setup the GPU mesh, don't keep the CPU mesh.
		level.upload();
		level.clearGeometry();
	}
	_bvhRange = glm::vec2(0, 0);
	checkGPUError();
}

void BVHRenderer::draw(const Camera & camera, Framebuffer & framebuffer, size_t layer) {

	// Draw the scene.
	GPU::setDepthState(true, TestFunction::LESS, true);
	GPU::setCullState(false);
	GPU::setBlendState(false);

	framebuffer.bind(layer, 0, glm::vec4(0.0f), 1.0f);
	framebuffer.setViewport();

	const glm::mat4 & view = camera.view();
	const glm::mat4 & proj = camera.projection();
	const glm::mat4 VP	 = proj * view;

	_objectProgram->use();
	_objectProgram->uniform("lightDir", glm::vec3(0.577f));
	for(auto & object : _scene->objects) {
		// Combine the three matrices.
		const glm::mat4 MVP			 = VP * object.model();
		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(object.model())));
		_objectProgram->uniform("mvp", MVP);
		_objectProgram->uniform("normalMatrix", glm::mat4(normalMatrix));
		GPU::drawMesh(*object.mesh());
	}

	// Debug wireframe visualisation.
	GPU::setPolygonState(PolygonMode::LINE);
	_bvhProgram->use();
	_bvhProgram->uniform("mvp", VP);
	// If there is a ray mesh, show it.
	if(_rayVis.gpu && _rayVis.gpu->count > 0) {
		GPU::drawMesh(_rayVis);
		if(_showBVH) {
			for(int lid = _bvhRange.x; lid <= _bvhRange.y; ++lid) {
				if(lid >= int(_rayLevels.size())) {
					break;
				}
				GPU::drawMesh(_rayLevels[lid]);
			}
		}
	} else if(_showBVH) {
		for(int lid = _bvhRange.x; lid <= _bvhRange.y; ++lid) {
			GPU::drawMesh(_bvhLevels[lid]);
		}
	}

	GPU::setPolygonState(PolygonMode::FILL);
	
}

BVHRenderer::~BVHRenderer() {
	for(Mesh & level : _bvhLevels) {
		level.clean();
	}
	for(Mesh & level : _rayLevels) {
		level.clean();
	}
	_rayVis.clean();
}

void BVHRenderer::castRay(const glm::vec3 & position, const glm::vec3 & direction) {

	// Intersect.
	const Raycaster::Hit hit = _visuHelper->getRayLevels(position, direction, _rayLevels);
	// Level meshes.
	for(Mesh & level : _rayLevels) {
		// Setup the GPU mesh, don't keep the CPU mesh.
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
