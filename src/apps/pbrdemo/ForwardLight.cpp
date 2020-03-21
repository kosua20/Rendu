#include "ForwardLight.hpp"
#include "graphics/GLUtilities.hpp"

ForwardLight::ForwardLight(size_t count) {
	_lightsData.resize(count);
}

void ForwardLight::updateCameraInfos(const glm::mat4 & viewMatrix, const glm::mat4 & projMatrix) {
	_view = viewMatrix;
	_proj = projMatrix;
}

void ForwardLight::updateShadowMapInfos(ShadowMode mode, float bias) {
	_shadowBias = bias;
	_shadowMode = mode;
}

void ForwardLight::draw(const SpotLight * light) {

	GPULight & currentLight = _lightsData[_currentId];
	_currentId				= (_currentId + 1) % _lightsData.size();

	const glm::vec3 lightPositionViewSpace	= glm::vec3(_view * glm::vec4(light->position(), 1.0f));
	const glm::vec3 lightDirectionViewSpace = glm::vec3(_view * glm::vec4(light->direction(), 0.0f));

	currentLight.viewToLight	   = light->vp() * glm::inverse(_view);
	currentLight.colorAndBias	   = glm::vec4(light->intensity(), _shadowBias);
	currentLight.positionAndRadius = glm::vec4(lightPositionViewSpace, light->radius());
	currentLight.directionAndPlane = glm::vec4(lightDirectionViewSpace, 0.0f);
	currentLight.anglesCos		   = glm::cos(light->angles());
	currentLight.type			   = int(LightType::POINT);
	currentLight.shadowMode		   = int(light->castsShadow() ? _shadowMode : ShadowMode::NONE);
}

void ForwardLight::draw(const PointLight * light) {
	GPULight & currentLight = _lightsData[_currentId];
	_currentId				= (_currentId + 1) % _lightsData.size();

	const glm::vec3 lightPositionViewSpace = glm::vec3(_view * glm::vec4(light->position(), 1.0f));

	currentLight.viewToLight		  = glm::inverse(_view);
	currentLight.colorAndBias		  = glm::vec4(light->intensity(), _shadowBias);
	currentLight.directionAndPlane[3] = light->farPlane();
	currentLight.positionAndRadius	  = glm::vec4(lightPositionViewSpace, light->radius());
	currentLight.type				  = int(LightType::POINT);
	currentLight.shadowMode			  = int(light->castsShadow() ? _shadowMode : ShadowMode::NONE);
}

void ForwardLight::draw(const DirectionalLight * light) {
	GPULight & currentLight = _lightsData[_currentId];
	_currentId				= (_currentId + 1) % _lightsData.size();

	const glm::vec3 lightDirectionViewSpace = glm::vec3(_view * glm::vec4(light->direction(), 0.0));

	currentLight.viewToLight	   = light->vp() * glm::inverse(_view);
	currentLight.colorAndBias	   = glm::vec4(light->intensity(), _shadowBias);
	currentLight.directionAndPlane = glm::vec4(lightDirectionViewSpace, 0.0f);
	currentLight.type			   = int(LightType::DIRECTIONAL);
	currentLight.shadowMode		   = int(light->castsShadow() ? _shadowMode : ShadowMode::NONE);
}
