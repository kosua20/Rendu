#include <stdio.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"

Camera::Camera(){
	_eye = glm::vec3(0.0,0.0,1.0);
	_center = glm::vec3(0.0,0.0,0.0);
	_up = glm::vec3(0.0,1.0,0.0);
	_view = glm::lookAt(_eye, _center, _up);
} 

Camera::~Camera(){}


void Camera::update(float elapsedTime){
}




