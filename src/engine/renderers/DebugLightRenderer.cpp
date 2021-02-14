#include "DebugLightRenderer.hpp"
#include "graphics/GPU.hpp"

DebugLightRenderer::DebugLightRenderer(const std::string & fragmentShader){
	_sphere  = Resources::manager().getMesh("light_sphere", Storage::GPU);
	_cone  = Resources::manager().getMesh("light_cone", Storage::GPU);
	_arrow  = Resources::manager().getMesh("light_arrow", Storage::GPU);
	_program = Resources::manager().getProgram(fragmentShader, "object_basic", fragmentShader);
}

void DebugLightRenderer::updateCameraInfos(const glm::mat4 & viewMatrix, const glm::mat4 & projMatrix){
	_view = viewMatrix;
	_proj = projMatrix;
}

void DebugLightRenderer::draw(const SpotLight * light) {
	const glm::mat4 mvp		 = _proj * _view * light->model();
	const glm::vec3 & color = light->intensity();
	const glm::vec3 colorLow = color / std::max(color[0], std::max(color[1], color[2]));
	
	_program->use();
	_program->uniform("mvp", mvp);
	_program->uniform("color", glm::vec4(colorLow, 1.0f));
	GPU::drawMesh(*_cone);
}

void DebugLightRenderer::draw(const PointLight * light) {
	const glm::mat4 mvp  = _proj * _view * light->model();
	const glm::mat4 mvp1 = glm::scale(mvp , glm::vec3(0.02f));
	
	const glm::vec3 & color  = light->intensity();
	const glm::vec3 colorLow = color / std::max(color[0], std::max(color[1], color[2]));
	
	_program->use();
	_program->uniform("mvp", mvp);
	_program->uniform("color", glm::vec4(colorLow, 1.0f));
	GPU::drawMesh(*_sphere);
	_program->uniform("mvp", mvp1);
	_program->uniform("color", glm::vec4(color, 1.0f));
	GPU::drawMesh(*_sphere);
}

void DebugLightRenderer::draw(const DirectionalLight * light) {
	const glm::mat4 vp		 = _proj * _view * light->model();
	const glm::vec3 & color = light->intensity();
	const glm::vec3 colorLow = color / std::max(color[0], std::max(color[1], color[2]));
	
	_program->use();
	_program->uniform("mvp", vp);
	_program->uniform("color", glm::vec4(colorLow, 1.0f));
	GPU::drawMesh(*_arrow);
	
}
