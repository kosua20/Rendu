#include <stdio.h>
#include <iostream>
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>
#include "../helpers/ResourcesManager.h"
#include <GLFW/glfw3.h>

#include "Joystick.h"

Joystick::Joystick(glm::vec3 & eye, glm::vec3 & center, glm::vec3 & up, glm::vec3 & right) : _eye(eye), _center(center), _up(up), _right(right){
	_speed = 1.0f;
	_angularSpeed = 4.0f;
	_id = -1;
	
	reset();
} 

Joystick::~Joystick(){}

void Joystick::activate(int id){
	_id = id;
	// Get axes and buttons references and count from GLFW
	_axes = glfwGetJoystickAxes(_id, &_axisCount);
	_buttons = glfwGetJoystickButtons(_id, &_buttonsCount);
	configure();
	reset();
}

void Joystick::deactivate(){
	_id = -1;
}

void Joystick::reset(){
	_recentPress[SPEED_UP] = false;
	_recentPress[SPEED_DOWN] = false;
	
}

void Joystick::update(double frameTime){
	// Update buttons flags.
	_axes = glfwGetJoystickAxes(_id, &_axisCount);
	_buttons = glfwGetJoystickButtons(_id, &_buttonsCount);
	
	// Handle buttons
	// Reset camera when pressing the Circle button.
	if(_buttons[_codes[RESET_ALL]] == GLFW_PRESS){
		_eye = glm::vec3(0.0,0.0,1.0);
		_center = glm::vec3(0.0,0.0,0.0);
		_up = glm::vec3(0.0,1.0,0.0);
		_right = glm::vec3(1.0,0.0,0.0);
		return;
	}
	
	// Special actions to restore the camera orientation.
	// Restore the up vector.
	if(_buttons[_codes[RESET_ORIENTATION]] == GLFW_PRESS){
		_up = glm::vec3(0.0f,1.0f,0.0f);
	}
	// Look at the center of the scene
	if( _buttons[_codes[RESET_CENTER]] == GLFW_PRESS){
		_center[0] = _center[1] = _center[2] = 0.0f;
	}
	
	// The Up and Down boutons are configured to register each press only once
	// to avoid increasing/decreasing the speed for as long as the button is pressed.
	if(_buttons[_codes[SPEED_UP]] == GLFW_PRESS){
		if(!_recentPress[SPEED_UP]){
			_speed *= 2.0f;
			std::cout << "Speed: " << _speed << std::endl;
			_recentPress[SPEED_UP] = true;
		}
	} else {
		_recentPress[SPEED_UP] = false;
	}
	
	if(_buttons[_codes[SPEED_DOWN]] == GLFW_PRESS){
		if(!_recentPress[SPEED_DOWN]){
			_speed *= 0.5f;
			std::cout << "Speed: " << _speed << std::endl;
			_recentPress[SPEED_DOWN] = true;
		}
	} else {
		_recentPress[SPEED_DOWN] = false;
	}
	
	// Handle axis
	// Left stick to move
	// We need the direction of the camera, normalized.
	glm::vec3 look = normalize(_center - _eye);
	// Require a minimum deplacement between starting to register the move.
	if(_axes[_codes[MOVE_FORWARD]]*_axes[_codes[MOVE_FORWARD]] + _axes[_codes[MOVE_LATERAL]]*_axes[_codes[MOVE_LATERAL]] > 0.1){
		// Update the camera position.
		_eye = _eye - _axes[_codes[MOVE_FORWARD]] * (float)frameTime * _speed * look;
		_eye = _eye + _axes[_codes[MOVE_LATERAL]] * (float)frameTime * _speed * _right;
	}
	
	// L2 and R2 triggers are used to move up and down. They can be read like axis.
	if(_axes[_codes[MOVE_UP]] > -0.9){
		_eye = _eye  - (_axes[_codes[MOVE_UP]]+1.0f)* 0.5f * (float)frameTime * _speed * _up;
	}
	if(_axes[_codes[MOVE_DOWN]] > -0.9){
		_eye = _eye  + (_axes[_codes[MOVE_DOWN]]+1.0f)* 0.5f * (float)frameTime * _speed * _up;
	}
	
	// Update center (eye-center stays constant).
	_center = _eye + look;
	
	// Right stick to look around.
	if(_axes[_codes[LOOK_VERTICAL]]*_axes[_codes[LOOK_VERTICAL]] + _axes[_codes[LOOK_LATERAL]]*_axes[_codes[LOOK_LATERAL]] > 0.1){
		_center = _center - _axes[_codes[LOOK_VERTICAL]] * (float)frameTime * _angularSpeed * _up;
		_center = _center + _axes[_codes[LOOK_LATERAL]] * (float)frameTime * _angularSpeed * _right;
	}
	// Renormalize the look vector.
	look = normalize(_center - _eye);
	// Recompute right as the cross product of look and up.
	_right = normalize(cross(look,_up));
	// Recompute up as the cross product of  right and look.
	_up = normalize(cross(_right,look));
}

void Joystick::configure(){
	
	// Register all keys by hand for now.
	// PS4 codes: from 0 to 5
	// L horizontal, L vertical, R horizontal, R vertical, L2, R2
	// PS4 codes: from 0 to 17
	// Square, Cross, Circle, Triangle, L1, R1, L2, R2, Share, Option, L3, R3, PS button, Touchpad, Up, Right, Down, Left
	const std::string settingsContent = Resources::manager().getTextFile("Controller.map");
	// If no mapping found, disable the controller.
	if(settingsContent.empty()){
		std::cerr << "No settings found for the controller." << std::endl;
		_id = -1;
		return;
	}
	// Parse the config file and update the map with it.
	_codes.clear();
	
	std::istringstream lines(settingsContent);
	std::string line;
	while(std::getline(lines,line)){
		// Split line at colon.
		const size_t separatorPos = line.find_first_of(":");
		// If no colon, skip line.
		if(separatorPos == std::string::npos){
			continue;
		}
		const std::string key = trim(trim(line.substr(0, separatorPos), " "), "\t");
		const int val = std::stoi(line.substr(separatorPos + 1));
		if(key == "MOVE_FORWARD") {
			_codes[MOVE_FORWARD] = val;
		} else if(key == "MOVE_LATERAL") {
			_codes[MOVE_LATERAL] = val;
		} else if(key == "LOOK_VERTICAL") {
			_codes[LOOK_VERTICAL] = val;
		} else if(key == "LOOK_LATERAL") {
			_codes[LOOK_LATERAL] = val;
		} else if(key == "MOVE_UP") {
			_codes[MOVE_UP] = val;
		} else if(key == "MOVE_DOWN") {
			_codes[MOVE_DOWN] = val;
		} else if(key == "RESET_ALL") {
			_codes[RESET_ALL] = val;
		} else if(key == "RESET_CENTER") {
			_codes[RESET_CENTER] = val;
		} else if(key == "RESET_ORIENTATION") {
			_codes[RESET_ORIENTATION] = val;
		} else if(key == "SPEED_UP") {
			_codes[SPEED_UP] = val;
		} else if(key == "SPEED_DOWN") {
			_codes[SPEED_DOWN] = val;
		} else {
			// Unknown key, ignore.
			std::cerr << "Unknown key: " << key << "." << std::endl;
		}
	}

}


std::string Joystick::trim(const std::string & str, const std::string & del){
	const size_t firstNotDel = str.find_first_not_of(del);
	if(firstNotDel == std::string::npos){
		return "";
	}
	const size_t lastNotDel = str.find_last_not_of(del);
	return str.substr(firstNotDel, lastNotDel - firstNotDel + 1);
}

