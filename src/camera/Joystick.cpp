#include <GLFW/glfw3.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <glm/gtc/matrix_transform.hpp>

#include "Joystick.h"

Joystick::Joystick(glm::vec3 & eye, glm::vec3 & center, glm::vec3 & up, glm::vec3 & right) : _eye(eye), _center(center), _up(up), _right(right){
	_speed = 1.0f;
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
	_recentPress[14] = false;
	_recentPress[16] = false;
}

void Joystick::update(float elapsedTime){
	// Update buttons flags.
	_axes = glfwGetJoystickAxes(_id, &_axisCount);
	_buttons = glfwGetJoystickButtons(_id, &_buttonsCount);
	
	// Handle buttons
	// PS4 codes: from 0 to 17
	// Square, Cross, Circle, Triangle, L1, R1, L2, R2, Share, Option, L3, R3, PS button, Touchpad, Up, Right, Down, Left
	// Reset camera when pressing the Circle button.
	if(_buttons[2] == GLFW_PRESS){
		_eye = glm::vec3(0.0,0.0,1.0);
		_center = glm::vec3(0.0,0.0,0.0);
		_up = glm::vec3(0.0,1.0,0.0);
		_right = glm::vec3(1.0,0.0,0.0);
		return;
	}
	
	// Special actions to restore the camera orientation.
	// Restore the up vector.
	if(_buttons[4] == GLFW_PRESS){
		_up = glm::vec3(0.0f,1.0f,0.0f);
	}
	// Look at the center of the scene
	if( _buttons[5] == GLFW_PRESS){
		_center[0] = _center[1] = _center[2] = 0.0f;
	}
	
	// The Up and Down boutons are configured to register each press only once
	// to avoid increasing/decreasing the speed for as long as the button is pressed.
	if(_buttons[14] == GLFW_PRESS){
		if(!_recentPress[14]){
			_speed *= 2.0f;
			std::cout << "Speed: " << _speed << std::endl;
			_recentPress[14] = true;
		}
	} else {
		_recentPress[14] = false;
	}
	
	if(_buttons[16] == GLFW_PRESS){
		if(!_recentPress[16]){
			_speed *= 0.5f;
			std::cout << "Speed: " << _speed << std::endl;
			_recentPress[16] = true;
		}
	} else {
		_recentPress[16] = false;
	}
	
	
	// Handle axis
	// PS4 codes: from 0 to 5
	// L horizontal, L vertical, R horizontal, R vertical, L2, R2
	
	// Left stick to move
	// We need the direction of the camera, normalized.
	glm::vec3 look = normalize(_center - _eye);
	// Require a minimum deplacement between starting to register the move.
	if(_axes[1]*_axes[1] + _axes[0]*_axes[0] > 0.1){
		// Update the camera position.
		_eye = _eye - _axes[1] * elapsedTime * _speed * look;
		_eye = _eye + _axes[0] * elapsedTime * _speed * _right;
	}
	
	// L2 and R2 triggers are used to move up and down. They can be read like axis.
	if(_axes[4] > -0.9){
		_eye = _eye  - (_axes[4]+1.0f)* 0.5f * elapsedTime * _speed * _up;
	}
	if(_axes[5] > -0.9){
		_eye = _eye  + (_axes[5]+1.0f)* 0.5f * elapsedTime * _speed * _up;
	}
	
	// Update center (eye-center stays constant).
	_center = _eye + look;
	
	// Right stick to look around.
	if(_axes[3]*_axes[3] + _axes[2]*_axes[2] > 0.1){
		_center = _center - _axes[3] * elapsedTime * _angularSpeed * _up;
		_center = _center + _axes[2] * elapsedTime * _angularSpeed * _right;
	}
	// Renormalize the look vector.
	look = normalize(_center - _eye);
	// Recompute right as the cross product of look and up.
	_right = normalize(cross(look,_up));
	// Recompute up as the cross product of  right and look.
	_up = normalize(cross(_right,look));
}

void Joystick::configure(){
	
}

