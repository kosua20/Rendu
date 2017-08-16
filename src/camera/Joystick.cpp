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
	
	configure();
	
	reset();
} 

Joystick::~Joystick(){}

void Joystick::activate(int id){
	_id = id;
	// Get axes and buttons references and count from GLFW
	_axes = glfwGetJoystickAxes(_id, &_axisCount);
	_buttons = glfwGetJoystickButtons(_id, &_buttonsCount);
	
	reset();
}

void Joystick::deactivate(){
	_id = -1;
}

void Joystick::reset(){
	
}

void Joystick::update(float elapsedTime){
	// Update buttons flags.
	_axes = glfwGetJoystickAxes(_id, &_axisCount);
	_buttons = glfwGetJoystickButtons(_id, &_buttonsCount);
	
	// Handle buttons
	// ...
	std::cout << "Buttons:";
	for(size_t i = 0; i < _buttonsCount; ++i){
		std::cout << " " << i << ":" << int(_buttons[i]);
	}
	std::cout << std::endl;
	
	// Handle axis
	// ...
	std::cout << "Axis:";
	for(size_t i = 0; i < _axisCount; ++i){
		std::cout << " " << i << ":" << _axes[i];
	}
	std::cout << std::endl;
}

void Joystick::configure(){
	
}

