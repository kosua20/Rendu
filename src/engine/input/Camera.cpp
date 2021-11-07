#include "input/Camera.hpp"
#include "resources/Bounds.hpp"
#include <sstream>

Camera::Camera() {
	updateView();
	updateProjection();
}

void Camera::pose(const glm::vec3 & position, const glm::vec3 & center, const glm::vec3 & up) {
	_eye					= position;
	_center					= center;
	_up						= glm::normalize(up);
	const glm::vec3 viewDir = glm::normalize(_center - _eye);
	_right					= glm::cross(viewDir, _up);
	_up						= glm::cross(_right, viewDir);
	updateView();
}

void Camera::projection(float ratio, float fov, float near, float far) {
	_clippingPlanes = glm::vec2(near, far);
	_ratio			= ratio;
	_fov			= fov;
	updateProjection();
}

void Camera::frustum(float near, float far) {
	_clippingPlanes = glm::vec2(near, far);
	updateProjection();
}

void Camera::ratio(float ratio) {
	_ratio = ratio;
	updateProjection();
}

void Camera::fov(float fov) {
	_fov = fov;
	updateProjection();
}

void Camera::pixelShifts(glm::vec3 & corner, glm::vec3 & dx, glm::vec3 & dy) const {
	const float heightScale = std::tan(0.5f * _fov);
	const float widthScale  = _ratio * heightScale;
	const float imageDist   = glm::distance(_eye, _center);
	corner					= _center + imageDist * (-widthScale * _right + heightScale * _up);
	dx						= 2.0f * widthScale * imageDist * _right;
	dy						= -2.0f * heightScale * imageDist * _up;
}

void Camera::updateProjection() {
	// Perspective projection.
	_projection = Frustum::perspective(_fov, _ratio, _clippingPlanes[0], _clippingPlanes[1]);
}

void Camera::updateView() {
	_view = glm::lookAt(_eye, _center, _up);
}

void Camera::apply(const Camera & camera) {
	const glm::vec2 & planes = camera.clippingPlanes();
	this->pose(camera.position(), camera.center(), camera.up());
	this->projection(camera.ratio(), camera.fov(), planes[0], planes[1]);
}

bool Camera::decode(const KeyValues & params) {
	glm::vec3 pos(0.0f, 0.0f, 1.0f);
	glm::vec3 center(0.0f, 0.0f, 0.0f);
	glm::vec3 up(0.0f, 1.0f, 0.0f);
	glm::vec2 planes(0.01f, 100.0f);
	float fov = 1.3f;

	const auto & elems = params.elements;
	for(const auto & param : elems) {
		if(param.key == "position") {
			pos = Codable::decodeVec3(param);
		} else if(param.key == "center") {
			center = Codable::decodeVec3(param);
		} else if(param.key == "up") {
			up = Codable::decodeVec3(param);
		} else if(param.key == "fov" && !param.values.empty()) {
			fov = std::stof(param.values[0]);
		} else if(param.key == "planes") {
			planes = Codable::decodeVec2(param);
		} else {
			Codable::unknown(param);
		}
	}
	// Validation:
	if(fov > glm::pi<float>()){
		Log::Error() << "Camera field of view is greater than 180°: " << fov << std::endl;
		return false;
	}
	if(planes[0] >= planes[1] || planes[0] <= 0.0f || planes[1] <= 0.0f){
		Log::Error() << "Camera planes are invalid: " << planes << std::endl;
		return false;
	}
	if((std::abs(glm::dot(glm::normalize(up), glm::normalize(pos - center))) > 0.99f)
	   || center == pos || up == glm::vec3(0.0f)){
		Log::Error() << "Camera look at configuration is invalid." << std::endl;
		return false;
	}
	// Apply the new pose and parameters.
	this->pose(pos, center, up);
	this->projection(_ratio, fov, planes.x, planes.y);
	return true;
}

KeyValues Camera::encode() const {
	KeyValues cam("camera");
	cam.elements.emplace_back("position");
	cam.elements.back().values = Codable::encode(_eye);
	cam.elements.emplace_back("center");
	cam.elements.back().values = Codable::encode(_center);
	cam.elements.emplace_back("up");
	cam.elements.back().values = Codable::encode(_up);
	cam.elements.emplace_back("fov");
	cam.elements.back().values = {std::to_string(_fov)};
	cam.elements.emplace_back("planes");
	cam.elements.back().values = Codable::encode(_clippingPlanes);
	return cam;
}
