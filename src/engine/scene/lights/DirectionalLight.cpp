#include "scene/lights/DirectionalLight.hpp"
#include "graphics/GLUtilities.hpp"

DirectionalLight::DirectionalLight(const glm::vec3 & worldDirection, const glm::vec3 & color) :
	Light(color),
	_lightDirection(glm::normalize(worldDirection)) {
}

void DirectionalLight::draw(LightRenderer & renderer) {
	renderer.draw(this);
}

void DirectionalLight::update(double fullTime, double frameTime) {
	glm::vec4 direction = glm::vec4(_lightDirection.get(), 0.0f);
	for(auto & anim : _animations) {
		direction = anim->apply(direction, fullTime, frameTime);
	}
	_lightDirection = glm::normalize(glm::vec3(direction));
	setScene(_sceneBox);
}

void DirectionalLight::setScene(const BoundingBox & sceneBox) {
	_sceneBox						 = sceneBox;
	const BoundingSphere sceneSphere = _sceneBox.getSphere();
	const glm::vec3 lightPosition	= sceneSphere.center - sceneSphere.radius * 1.1f * _lightDirection.get();
	const glm::vec3 lightTarget		 = sceneSphere.center;
	_viewMatrix						 = glm::lookAt(lightPosition, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));

	const BoundingBox lightSpacebox = _sceneBox.transformed(_viewMatrix);
	const float absz1				= std::abs(lightSpacebox.minis[2]);
	const float absz2				= std::abs(lightSpacebox.maxis[2]);
	const float near				= std::min(absz1, absz2);
	const float far					= std::max(absz1, absz2);
	const float scaleMargin			= 1.5f;
	_projectionMatrix				= glm::ortho(scaleMargin * lightSpacebox.minis[0], scaleMargin * lightSpacebox.maxis[0], scaleMargin * lightSpacebox.minis[1], scaleMargin * lightSpacebox.maxis[1], (1.0f / scaleMargin) * near, scaleMargin * far);
	_vp = _projectionMatrix * _viewMatrix;
	_model = glm::inverse(_viewMatrix) * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f));
}

glm::vec3 DirectionalLight::sample(const glm::vec3 & , float & dist, float & attenuation) const {
	attenuation = 1.0f;
	dist = std::numeric_limits<float>::max();
	return -_lightDirection.get();
}

void DirectionalLight::decode(const KeyValues & params) {
	Light::decodeBase(params);
	for(const auto & param : params.elements) {
		if(param.key == "direction") {
			_lightDirection.reset(glm::normalize(Codable::decodeVec3(param)));
		}
	}
}

KeyValues DirectionalLight::encode() const {
	KeyValues light = Light::encode();
	light.key = "directional";
	light.elements.emplace_back("direction");
	light.elements.back().values = Codable::encode(_lightDirection.initial());
	return light;
}
