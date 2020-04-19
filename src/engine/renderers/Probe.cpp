#include "Probe.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GLUtilities.hpp"

Probe::Probe(const glm::vec3 & position, std::shared_ptr<Renderer> renderer, unsigned int size, const glm::vec2 & clippingPlanes){
	_renderer = renderer;
	_framebuffer = renderer->createOutput(TextureShape::Cube, size, size, 6);
	_framebuffer->clear(glm::vec4(0.0f), 1.0f);
	_position = position;

	// Compute the camera for each face.
	const std::array<glm::vec3, 6> ups = {
		glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)
	};
	const glm::vec3 centers[6] = {glm::vec3(1.0, 0.0, 0.0), glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 0.0, -1.0)};
	for(uint i = 0; i < 6; ++i){
		_cameras[i].pose(position, position + centers[i], ups[i]);
		_cameras[i].projection(1.0f, glm::half_pi<float>(), clippingPlanes[0], clippingPlanes[1]);
	}
}

void Probe::draw(){
	for(uint i = 0; i < 6; ++i){
		_renderer->draw(_cameras[i], *_framebuffer, i);
	}
}

void Probe::clean(){
	_framebuffer->clean();
}
