#include <stdio.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"

Camera::Camera(){
	reset();
} 

Camera::~Camera(){}

void Camera::reset(){
	_eye = glm::vec3(0.0,0.0,1.0);
	_center = glm::vec3(0.0,0.0,0.0);
	_up = glm::vec3(0.0,1.0,0.0);
	_view = glm::lookAt(_eye, _center, _up);
}

void Camera::update(float elapsedTime){
}

void Camera::registerMove(int direction, bool flag){
	std::cout << (flag ? "Begin " : "End " ) << direction << std::endl;
}




