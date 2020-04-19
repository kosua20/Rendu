#include "Probe.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GLUtilities.hpp"
#include "resources/ResourcesManager.hpp"


Probe::Probe(const glm::vec3 & position, std::shared_ptr<Renderer> renderer, uint size, uint mips, const glm::vec2 & clippingPlanes){
	_renderer = renderer;
	_framebuffer = renderer->createOutput(TextureShape::Cube, size, size, 6, mips);
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
		// Generate centered MVPs for post-process steps.
		_mvps[i] = glm::perspective(glm::half_pi<float>(), 1.0f, 0.01f, 10.0f) * glm::lookAt(glm::vec3(0.0f), centers[i], ups[i]);
	}
}

void Probe::draw(){
	for(uint i = 0; i < 6; ++i){
		_renderer->draw(_cameras[i], *_framebuffer, i);
	}
}

void Probe::integrate(float clamp){

	const auto _integration = Resources::manager().getProgram("cubemap_convo", "skybox_basic", "cubemap_convo");
	const auto mesh			= Resources::manager().getMesh("skybox", Storage::GPU);

	GLUtilities::setDepthState(false);
	GLUtilities::setBlendState(false);
	GLUtilities::setCullState(true);

	_integration->use();
	_integration->uniform("clampMax", clamp);

	for(uint mid = 1; mid < _framebuffer->textureId()->levels; ++mid){
		const uint wh = _framebuffer->textureId()->width / (1 << mid);
		const float roughness = float(mid) / float((_framebuffer->textureId()->levels)-1);
		const int samplesCount = (mid == 1 ? 64 : 128);

		GLUtilities::setViewport(0, 0, int(wh), int(wh));
		_integration->uniform("mipmapRoughness", roughness);
		_integration->uniform("samplesCount", samplesCount);

		for(uint lid = 0; lid < 6; ++lid){
			_framebuffer->bind(lid, mid);
			_integration->uniform("mvp", _mvps[lid]);
			GLUtilities::bindTexture(_framebuffer->textureId(), 0);
			GLUtilities::drawMesh(*mesh);
		}
	}
	_framebuffer->unbind();
}

void Probe::clean(){
	_framebuffer->clean();
}
