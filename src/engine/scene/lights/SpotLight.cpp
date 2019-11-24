#include "scene/lights/SpotLight.hpp"
#include "graphics/GLUtilities.hpp"

SpotLight::SpotLight(const glm::vec3 & worldPosition, const glm::vec3 & worldDirection, const glm::vec3 & color, float innerAngle, float outerAngle, float radius) :
	Light(color),
	_lightDirection(glm::normalize(worldDirection)), _lightPosition(worldPosition),
	_angles(0.5f * innerAngle, 0.5f * outerAngle), _radius(radius) {
}

void SpotLight::draw(const LightRenderer & renderer) const {
	renderer.draw(this);
}

void SpotLight::update(double fullTime, double frameTime) {
	glm::vec4 position  = glm::vec4(_lightPosition.get(), 1.0f);
	glm::vec4 direction = glm::vec4(_lightDirection.get(), 0.0f);
	for(auto & anim : _animations) {
		position  = anim->apply(position, fullTime, frameTime);
		direction = anim->apply(direction, fullTime, frameTime);
	}
	_lightPosition  = glm::vec3(position);
	_lightDirection = glm::normalize(glm::vec3(direction));
	setScene(_sceneBox);
}

void SpotLight::setScene(const BoundingBox & sceneBox) {
	_sceneBox   = sceneBox;
	_viewMatrix = glm::lookAt(_lightPosition.get(), _lightPosition.get() + _lightDirection.get(), glm::vec3(0.0f, 1.0f, 0.0f));

	// Compute the projection matrix, automatically finding the near and far.
	float near;
	float far;
	if(_sceneBox.contains(_lightPosition)) {
		const float size = glm::length(_sceneBox.getSize());
		near			 = 0.01f * size;
		far				 = 1.0f * size;
	} else {
		const BoundingBox lightSpacebox = _sceneBox.transformed(_viewMatrix);
		const float absz1				= std::abs(lightSpacebox.minis[2]);
		const float absz2				= std::abs(lightSpacebox.maxis[2]);
		near							= std::min(absz1, absz2);
		far								= std::max(absz1, absz2);
	}
	_projectionMatrix = glm::perspective(2.0f * _angles.y, 1.0f, near, far);
	_vp			  	  = _projectionMatrix * _viewMatrix;
	// Compute the model matrix to scale the cone based on the outer angle and the radius.
	const float width			= 2.0f * std::tan(_angles.y);
	_model = glm::inverse(_viewMatrix) * glm::scale(glm::mat4(1.0f), _radius * glm::vec3(width, width, 1.0f));
}

bool SpotLight::visible(const glm::vec3 & position, const Raycaster & raycaster, glm::vec3 & direction, float & attenuation) const {
	if(_castShadows && !raycaster.visible(position, _lightPosition)) {
		return false;
	}
	direction = _lightPosition.get() - position;

	// Early exit if we are outside the sphere of influence.
	const float localRadius = glm::length(direction);
	if(localRadius > _radius) {
		return false;
	}
	if(localRadius > 0.0f) {
		direction /= localRadius;
	}

	// Compute the angle between the light direction and the (light, surface point) vector.
	const float currentCos = glm::dot(-direction, _lightDirection.get());
	const float outerCos   = std::cos(_angles.y);
	// If we are outside the spotlight cone, no lighting.
	if(currentCos < std::cos(outerCos)) {
		return false;
	}
	// Compute the spotlight attenuation factor based on our angle compared to the inner and outer spotlight angles.
	const float innerCos		 = std::cos(_angles.x);
	const float angleAttenuation = glm::clamp((currentCos - outerCos) / (innerCos - outerCos), 0.0f, 1.0f);

	// Attenuation with increasing distance to the light.
	const float radiusRatio  = localRadius / _radius;
	const float radiusRatio2 = radiusRatio * radiusRatio;
	const float attenNum	 = glm::clamp(1.0f - radiusRatio2, 0.0f, 1.0f);
	attenuation				 = angleAttenuation * attenNum * attenNum;
	return true;
}

void SpotLight::decode(const KeyValues & params) {
	Light::decodeBase(params);
	for(const auto & param : params.elements) {
		if(param.key == "direction") {
			_lightDirection.reset(glm::normalize(Codable::decodeVec3(param)));

		} else if(param.key == "position") {
			_lightPosition.reset(Codable::decodeVec3(param));

		} else if(param.key == "cone" && param.values.size() >= 2) {
			const float innerAngle = std::stof(param.values[0]);
			const float outerAngle = std::stof(param.values[1]);
			_angles = 0.5f * glm::vec2(innerAngle, outerAngle);

		} else if(param.key == "radius" && !param.values.empty()) {
			_radius = std::stof(param.values[0]);
		}
	}
}

KeyValues SpotLight::encode() const {
	KeyValues light = Light::encode();
	light.key = "spot";
	light.elements.emplace_back("position");
	light.elements.back().values = { Codable::encode(_lightPosition.initial()) };
	light.elements.emplace_back("direction");
	light.elements.back().values = { Codable::encode(_lightDirection.initial()) };
	light.elements.emplace_back("radius");
	light.elements.back().values = { std::to_string(_radius) };
	light.elements.emplace_back("cone");
	light.elements.back().values = { std::to_string(_angles[0] * 2.0f), std::to_string(_angles[1] * 2.0f) };
	return light;
}
