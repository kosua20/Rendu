#include "scene/lights/PointLight.hpp"
#include "graphics/GLUtilities.hpp"
#include "resources/ResourcesManager.hpp"

PointLight::PointLight(const glm::vec3 & worldPosition, const glm::vec3 & color, float radius) :
	Light(color),
	_lightPosition(worldPosition), _radius(radius) {
}

void PointLight::draw(LightRenderer & renderer) {
	renderer.draw(this);
}

void PointLight::update(double fullTime, double frameTime) {
	glm::vec4 position = glm::vec4(_lightPosition.get(), 0.0);
	for(auto & anim : _animations) {
		position = anim->apply(position, fullTime, frameTime);
	}
	_lightPosition = glm::vec3(position);
	setScene(_sceneBox);
}

void PointLight::setScene(const BoundingBox & sceneBox) {
	_sceneBox = sceneBox;

	const glm::mat4 model = glm::translate(glm::mat4(1.0f), -_lightPosition.get());

	// Compute the projection matrix based on the scene bounding box.
	// As both the view matrices and the bounding boxe are axis aligned, we can avoid costly transformations.
	const glm::vec3 deltaMini = _lightPosition.get() - _sceneBox.minis;
	const glm::vec3 deltaMaxi = _lightPosition.get() - _sceneBox.maxis;
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
	_model = glm::scale(glm::translate(glm::mat4(1.0f), _lightPosition.get()), glm::vec3(_radius));
}


glm::vec3 PointLight::sample(const glm::vec3 & position, float & dist, float & attenuation) const {
	glm::vec3 direction = _lightPosition.get() - position;
	dist = glm::length(direction);
	attenuation = 0.0f;
	// Early exit if we are outside the sphere of influence.
	if(dist > _radius) {
		return {};
	}
	if(dist > 0.0f) {
		direction /= dist;
	}

	// Attenuation with increasing distance to the light.
	const float radiusRatio  = dist / _radius;
	const float radiusRatio2 = radiusRatio * radiusRatio;
	const float attenNum	 = glm::clamp(1.0f - radiusRatio2, 0.0f, 1.0f);
	attenuation  = attenNum * attenNum;

	return direction;
}

void PointLight::decode(const KeyValues & params) {
	Light::decodeBase(params);
	for(const auto & param : params.elements) {
		if(param.key == "position") {
			_lightPosition.reset(Codable::decodeVec3(param));
		} else if(param.key == "radius" && !param.values.empty()) {
			_radius = std::stof(param.values[0]);
		}
	}
}

KeyValues PointLight::encode() const {
	KeyValues light = Light::encode();
	light.key = "point";
	light.elements.emplace_back("position");
	light.elements.back().values = { Codable::encode(_lightPosition.initial()) };
	light.elements.emplace_back("radius");
	light.elements.back().values = { std::to_string(_radius) };
	return light;
}
