#include <GLFW/glfw3.h>
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
	_right = glm::vec3(1.0,0.0,0.0);

	_view = glm::lookAt(_eye, _center, _up);
	_keys[0] = _keys[1] = _keys[2] = _keys[3] = false;
	_speed = 1.2;
}

void Camera::update(float elapsedTime){
	// We need the direction of the camera, normalized.
	glm::vec3 look = normalize(_center - _eye);
	// One step forward or backward.
	glm::vec3 deltaLook =  _speed * elapsedTime * look;
	// One step laterally.
	glm::vec3 deltaLateral = _speed * elapsedTime * _right;

	
	if(_keys[0]){ // Forward
		_eye = _eye + deltaLook;
	}

	if(_keys[1]){ // Backward
  		_eye = _eye - deltaLook;
	}

	if(_keys[2]){ // Left
  		_eye = _eye - deltaLateral;
	}

	if(_keys[3]){ // Right
  		_eye = _eye + deltaLateral;
	}

	// Update center (eye-center stays constant).
	_center = _eye + look;
  	
  	// Recompute right as the cross product of look and up.
	_right = normalize(cross(look,_up));
	// Recompute up as the cross product of  right and look.
	_up = normalize(cross(_right,look));

	// Update the view matrix.
	_view = glm::lookAt(_eye, _center, _up);
}

void Camera::registerMove(int direction, bool flag){
	// Depending on the direction, we update the corresponding flag. 
	// This will allow for smooth movements.
	switch(direction){
		case GLFW_KEY_W:
			_keys[0] = flag;
			break;
		case GLFW_KEY_S:
			_keys[1] = flag;
			break;
		case GLFW_KEY_A:
			_keys[2] = flag;
			break;
		case GLFW_KEY_D:
			_keys[3] = flag;
			break;
		default:
			break;
	}
}




