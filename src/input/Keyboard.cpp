#include <GLFW/glfw3.h>
#include <stdio.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#include "Keyboard.h"
/*
Keyboard::Keyboard(glm::vec3 & eye, glm::vec3 & center, glm::vec3 & up, glm::vec3 & right) : _eye(eye), _center(center), _up(up), _right(right) {
	_speed = 1.2f;
	_angularSpeed = 75.0f;
	reset();
} 

Keyboard::~Keyboard(){}

void Keyboard::reset(){
	_keys[0] = _keys[1] = _keys[2] = _keys[3] = _keys[4] = _keys[5] = _keys[6] = false;
	_previousPosition = glm::vec2(0.0);
	_deltaPosition = glm::vec2(0.0);
}

void Keyboard::update(double frameTime){
	
	

}

void Keyboard::registerMove(int direction, bool flag){
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
		case GLFW_KEY_Q:
			_keys[4] = flag;
			break;
		case GLFW_KEY_E:
			_keys[5] = flag;
			break;
		default:
			break;
	}
}

void Keyboard::startLeftMouse(double x, double y){
	_previousPosition = glm::vec2(x,-y);
}

void Keyboard::leftMouseTo(double x, double y){
	_keys[6] = true;
	_deltaPosition = glm::vec2(x, -y) - _previousPosition;
	_previousPosition =  glm::vec2(x, -y) ;
}

void Keyboard::endLeftMouse(){
	_keys[6] = false;
}

*/
