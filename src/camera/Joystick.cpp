#include <GLFW/glfw3.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <glm/gtc/matrix_transform.hpp>

#include "Joystick.h"

Joystick::Joystick(glm::vec3 & eye, glm::vec3 & center, glm::vec3 & up, glm::vec3 & right) : _eye(eye), _center(center), _up(up), _right(right){
	_speed = 2.5f;
	_angularSpeed = 4.0f;
	_id = -1;
	
	reset();
} 

Joystick::~Joystick(){}

void Joystick::activate(int id){
	_id = id;
}

void Joystick::deactivate(){
	_id = -1;
}

void Joystick::reset(){
	
}

void Joystick::update(float elapsedTime){
	
}

