#include "ForwardLight.hpp"
#include "graphics/GPU.hpp"

const size_t ForwardLight::_maxLightCount = 50;

ForwardLight::ForwardLight(size_t count) :
	_lightsData(_maxLightCount, UniformFrequency::VIEW) {
	_currentCount = count;
	if(_currentCount > _maxLightCount){
		Log::Warning() << "Forward light renderer can only handle the first " << _maxLightCount << " lights (requested " << _currentCount << ")." << std::endl;
	}

	// Initial buffers creation and allocation.
	_lightsData.upload();
	_shadowMaps.resize(2, nullptr);
}

void ForwardLight::updateCameraInfos(const glm::mat4 & viewMatrix, const glm::mat4 & projMatrix) {
	_view = viewMatrix;
	_proj = projMatrix;
	_invView = glm::inverse(_view);
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

	currentLight.viewToLight	   = light->vp() * _invView;
	currentLight.colorAndBias	   = glm::vec4(light->intensity(), _shadowBias);
	currentLight.positionAndRadius = glm::vec4(lightPositionViewSpace, light->radius());
	currentLight.directionAndPlane = glm::vec4(lightDirectionViewSpace, 0.0f);

	currentLight.typeModeLayer[0] = float(LightType::SPOT);
	currentLight.typeModeLayer[1] = float(light->castsShadow() ? _shadowMode : ShadowMode::NONE);
	currentLight.typeModeLayer[2] = float(light->shadowMap().layer);

	currentLight.angles[0] = glm::cos(light->angles()[0]);
	currentLight.angles[1] = glm::cos(light->angles()[1]);

	if(light->castsShadow()){
		_shadowMaps[0] = light->shadowMap().map;
	}
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

	currentLight.viewToLight		  = _invView;
	currentLight.colorAndBias		  = glm::vec4(light->intensity(), _shadowBias);
	currentLight.directionAndPlane[3] = light->farPlane();
	currentLight.positionAndRadius	  = glm::vec4(lightPositionViewSpace, light->radius());

	currentLight.typeModeLayer[0] = float(LightType::POINT);
	currentLight.typeModeLayer[1] = float(light->castsShadow() ? _shadowMode : ShadowMode::NONE);
	currentLight.typeModeLayer[2] = float(light->shadowMap().layer);

	if(light->castsShadow()){
		_shadowMaps[1] = light->shadowMap().map;
	}
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

	currentLight.viewToLight	   = light->vp() * _invView;
	currentLight.colorAndBias	   = glm::vec4(light->intensity(), _shadowBias);
	currentLight.directionAndPlane = glm::vec4(lightDirectionViewSpace, 0.0f);

	currentLight.typeModeLayer[0] = float(LightType::DIRECTIONAL);
	currentLight.typeModeLayer[1] = float(light->castsShadow() ? _shadowMode : ShadowMode::NONE);
	currentLight.typeModeLayer[2] = float(light->shadowMap().layer);

	if(light->castsShadow()){
		_shadowMaps[0] = light->shadowMap().map;
	}
}
