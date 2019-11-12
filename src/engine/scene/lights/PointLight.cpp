#include "scene/lights/PointLight.hpp"
#include "graphics/GLUtilities.hpp"
#include "resources/ResourcesManager.hpp"

PointLight::PointLight(const glm::vec3 & worldPosition, const glm::vec3 & color, float radius) :
	Light(color),
	_lightPosition(worldPosition), _radius(radius) {
}

void PointLight::draw(const LightRenderer & renderer) const {
	renderer.draw(this);
}

void PointLight::update(double fullTime, double frameTime) {
	glm::vec4 position = glm::vec4(_lightPosition, 0.0);
	for(auto & anim : _animations) {
		position = anim->apply(position, fullTime, frameTime);
	}
	_lightPosition = glm::vec3(position);
	setScene(_sceneBox);
}

void PointLight::setScene(const BoundingBox & sceneBox) {
	_sceneBox = sceneBox;

	const glm::mat4 model = glm::translate(glm::mat4(1.0f), -_lightPosition);

	// Compute the projection matrix based on the scene bounding box.
	// As both the view matrices and the bounding boxe are axis aligned, we can avoid costly transformations.
	const glm::vec3 deltaMini = _lightPosition - _sceneBox.minis;
	const glm::vec3 deltaMaxi = _lightPosition - _sceneBox.maxis;
	// Absolute value of each min/max  distance on each axis.
	const glm::vec3 candidatesNear = glm::min(glm::abs(deltaMini), glm::abs(deltaMaxi));
	const glm::vec3 candidatesFar  = glm::max(glm::abs(deltaMini), glm::abs(deltaMaxi));

	const float size = glm::length(_sceneBox.getSize());
	float far		 = candidatesFar[0];
	float near		 = candidatesNear[0];
	bool allInside   = true;
	for(int i = 0; i < 3; ++i) {
		// The light is inside the bbox along the axis i if the two delta have different signs.
		const bool isInside = (std::signbit(deltaMini[i]) != std::signbit(deltaMaxi[i]));
		allInside			= allInside && isInside;
		// In this case we enforce a small near.
		near = (std::min)(near, candidatesNear[i]);
		far  = (std::max)(far, candidatesFar[i]);
	}
	if(allInside) {
		near = 0.01f * size;
		far  = size;
	}
	_farPlane				   = far;
	const glm::mat4 projection = glm::perspective(glm::half_pi<float>(), 1.0f, near, _farPlane);

	// Create the constant view matrices for the 6 faces.
	const glm::vec3 ups[6]	 = {glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, -1.0, 0.0)};
	const glm::vec3 centers[6] = {glm::vec3(1.0, 0.0, 0.0), glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 0.0, -1.0)};

	// Udpate the VPs.
	for(size_t mid = 0; mid < 6; ++mid) {
		const glm::mat4 view = glm::lookAt(glm::vec3(0.0f), centers[mid], ups[mid]);
		_vps[mid]			 = projection * view * model;
	}
	
	// Compute the model matrix to scale the sphere based on the radius.
	_model = glm::scale(glm::translate(glm::mat4(1.0f), _lightPosition), glm::vec3(_radius));
}

bool PointLight::visible(const glm::vec3 & position, const Raycaster & raycaster, glm::vec3 & direction, float & attenuation) const {
	if(_castShadows && !raycaster.visible(position, _lightPosition)) {
		return false;
	}
	direction = _lightPosition - position;

	// Early exit if we are outside the sphere of influence.
	const float localRadius = glm::length(direction);
	if(localRadius > _radius) {
		return false;
	}
	if(localRadius > 0.0f) {
		direction /= localRadius;
	}

	// Attenuation with increasing distance to the light.
	const float radiusRatio  = localRadius / _radius;
	const float radiusRatio2 = radiusRatio * radiusRatio;
	const float attenNum	 = glm::clamp(1.0f - radiusRatio2, 0.0f, 1.0f);
	attenuation				 = attenNum * attenNum;
	return true;
}

void PointLight::decode(const KeyValues & params) {
	Light::decodeBase(params);
	for(const auto & param : params.elements) {
		if(param.key == "position") {
			_lightPosition = Codable::decodeVec3(param);
		} else if(param.key == "radius" && !param.values.empty()) {
			_radius = std::stof(param.values[0]);
		}
	}
}
