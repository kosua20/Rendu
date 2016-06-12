#include <GLFW/glfw3.h>
#include <stdio.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"


Camera::Camera() : _keyboard(_eye, _center, _up, _right) {
	reset();
}

Camera::~Camera(){}

void Camera::reset(){
	_eye = glm::vec3(0.0,0.0,1.0);
	_center = glm::vec3(0.0,0.0,0.0);
	_up = glm::vec3(0.0,1.0,0.0);
	_right = glm::vec3(1.0,0.0,0.0);
	_view = glm::lookAt(_eye, _center, _up);
	_keyboard.reset();
}

void Camera::update(float elapsedTime){
	
	_keyboard.update(elapsedTime);
	// Update the view matrix.
	_view = glm::lookAt(_eye, _center, _up);
}


void Camera::key(int key, bool flag){
	if (key == GLFW_KEY_W || key == GLFW_KEY_A
		|| key == GLFW_KEY_S || key == GLFW_KEY_D
		|| key == GLFW_KEY_Q || key == GLFW_KEY_E) {
		_keyboard.registerMove(key, flag);
	} else if(flag && key == GLFW_KEY_R) {
		reset();
	} else {
		std::cout << "Key: " << key << " (" << char(key) << ")." << std::endl;
	}
}

void Camera::mouse(MouseMode mode, float x, float y){
	if(mode == MouseMode::Start) {
		_keyboard.startLeftMouse(x,y);
	} else if (mode == MouseMode::End) {
		_keyboard.endLeftMouse();
	} else {
		_keyboard.leftMouseTo(x,y);
	}
}




