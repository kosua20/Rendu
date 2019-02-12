#include "Camera.hpp"
#include "Input.hpp"

Camera::Camera()  {
	_fov = 1.91f;
	_ratio = 4.0f/3.0f;
	_near = 0.01f;
	_far = 100.0f;
	_eye = glm::vec3(0.0,0.0,1.0);
	_center = glm::vec3(0.0,0.0,0.0);
	_up = glm::vec3(0.0,1.0,0.0);
	_right = glm::vec3(1.0,0.0,0.0);
	updateView();
	updateProjection();
}

void Camera::pose(const glm::vec3 & position, const glm::vec3 & center, const glm::vec3 & up){
	_eye = position;
	_center = center;
	_up = glm::normalize(up);
	_right = glm::cross(glm::normalize(_center - _eye), _up);
	updateView();
}

void Camera::projection(float ratio, float fov, float near, float far){
	_near = near;
	_far = far;
	_ratio = ratio;
	_fov = fov;
	updateProjection();
}

void Camera::frustum(float near, float far){
	_near = near;
	_far = far;
	updateProjection();
}

void Camera::ratio(float ratio){
	_ratio = ratio;
	updateProjection();
}

void Camera::fov(float fov){
	_fov = fov;
	updateProjection();
}

void Camera::updateProjection(){
	// Perspective projection.
	_projection = glm::perspective(_fov, _ratio, _near, _far);
}

void Camera::updateView(){
	_view = glm::lookAt(_eye, _center, _up);
}

