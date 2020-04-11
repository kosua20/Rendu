#include "ForwardLight.hpp"
#include "graphics/GLUtilities.hpp"

ForwardLight::ForwardLight(size_t count) {
	_currentCount = count;
	if(_currentCount > _maxLightCount){
		Log::Warning() << "Forward light renderer can only handle the first " << _maxLightCount << " lights (requested " << _currentCount << ")." << std::endl;
	}

	// Initially buffer creation and allocation.
	glGenBuffers(1, &_bufferHandle);
	glBindBuffer(GL_UNIFORM_BUFFER, _bufferHandle);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(GPULight)*_maxLightCount, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
	const size_t selectedId = _currentId;
	_currentId				= (_currentId + 1) % _currentCount;
	// Skip update if extraneous light.
	if(selectedId >= _maxLightCount) {
		return;
	}

	GPULight & currentLight					= _lightsData[selectedId];
	const glm::vec3 lightPositionViewSpace	= glm::vec3(_view * glm::vec4(light->position(), 1.0f));
	const glm::vec3 lightDirectionViewSpace = glm::vec3(_view * glm::vec4(light->direction(), 0.0f));

	currentLight.viewToLight	   = light->vp() * glm::inverse(_view);
	currentLight.colorAndBias	   = glm::vec4(light->intensity(), _shadowBias);
	currentLight.positionAndRadius = glm::vec4(lightPositionViewSpace, light->radius());
	currentLight.directionAndPlane = glm::vec4(lightDirectionViewSpace, 0.0f);

	currentLight.typeModeAngles[0] = float(LightType::SPOT);
	currentLight.typeModeAngles[1] = float(light->castsShadow() ? _shadowMode : ShadowMode::NONE);
	currentLight.typeModeAngles[2] = glm::cos(light->angles()[0]);
	currentLight.typeModeAngles[3] = glm::cos(light->angles()[1]);
}

void ForwardLight::draw(const PointLight * light) {
	const size_t selectedId = _currentId;
	_currentId				= (_currentId + 1) % _currentCount;
	// Skip update if extraneous light.
	if(selectedId >= _maxLightCount) {
		return;
	}

	GPULight & currentLight				   = _lightsData[selectedId];
	const glm::vec3 lightPositionViewSpace = glm::vec3(_view * glm::vec4(light->position(), 1.0f));

	currentLight.viewToLight		  = glm::inverse(_view);
	currentLight.colorAndBias		  = glm::vec4(light->intensity(), _shadowBias);
	currentLight.directionAndPlane[3] = light->farPlane();
	currentLight.positionAndRadius	  = glm::vec4(lightPositionViewSpace, light->radius());
	currentLight.typeModeAngles[0]	  = float(LightType::POINT);
	currentLight.typeModeAngles[1]	  = float(light->castsShadow() ? _shadowMode : ShadowMode::NONE);
}

void ForwardLight::draw(const DirectionalLight * light) {
	const size_t selectedId = _currentId;
	_currentId				= (_currentId + 1) % _currentCount;
	// Skip update if extraneous light.
	if(selectedId >= _maxLightCount) {
		return;
	}

	GPULight & currentLight					= _lightsData[selectedId];
	const glm::vec3 lightDirectionViewSpace = glm::vec3(_view * glm::vec4(light->direction(), 0.0));

	currentLight.viewToLight	   = light->vp() * glm::inverse(_view);
	currentLight.colorAndBias	   = glm::vec4(light->intensity(), _shadowBias);
	currentLight.directionAndPlane = glm::vec4(lightDirectionViewSpace, 0.0f);

	currentLight.typeModeAngles[0] = float(LightType::DIRECTIONAL);
	currentLight.typeModeAngles[1] = float(light->castsShadow() ? _shadowMode : ShadowMode::NONE);

}

void ForwardLight::upload() const {
	glBindBuffer(GL_UNIFORM_BUFFER, _bufferHandle);
	// Start by orphaning the buffer.
	glBufferData(GL_UNIFORM_BUFFER, sizeof(GPULight)*_maxLightCount, nullptr, GL_DYNAMIC_DRAW);
	// Then upload the data.
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GPULight)*_maxLightCount, &_lightsData[0]);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void ForwardLight::bind(size_t slot) const {
	glBindBuffer(GL_UNIFORM_BUFFER, _bufferHandle);
	glBindBufferBase(GL_UNIFORM_BUFFER, GLuint(slot), _bufferHandle);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
