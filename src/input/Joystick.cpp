#include <stdio.h>
#include <iostream>
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>
#include "../helpers/ResourcesManager.h"
#include <GLFW/glfw3.h>

#include "Joystick.h"

Joystick::Joystick() {
	_id = -1;
	_recentPress[SPEED_UP] = false;
	_recentPress[SPEED_DOWN] = false;
} 

Joystick::~Joystick(){}

bool Joystick::activate(int id){
	_id = id;
	// Get axes and buttons references and count from GLFW
	_axes = glfwGetJoystickAxes(_id, &_axisCount);
	_buttons = glfwGetJoystickButtons(_id, &_buttonsCount);
	_recentPress[SPEED_UP] = false;
	_recentPress[SPEED_DOWN] = false;

	return configure();
}

void Joystick::deactivate(){
	_id = -1;
}

void Joystick::update(){
	// Update buttons flags.
	_axes = glfwGetJoystickAxes(_id, &_axisCount);
	_buttons = glfwGetJoystickButtons(_id, &_buttonsCount);
	
	// Handle buttons
	// Reset camera when pressing the Circle button.
	/*if(_buttons[_codes[RESET_ALL]] == GLFW_PRESS){
		
		return;
	}*/
	
	// The Up and Down boutons are configured to register each press only once
	// to avoid increasing/decreasing the speed for as long as the button is pressed.
	/*if(_buttons[_codes[SPEED_UP]] == GLFW_PRESS){
		if(!_recentPress[SPEED_UP]){
			
			_recentPress[SPEED_UP] = true;
		}
	} else {
		_recentPress[SPEED_UP] = false;
	}*/
	
	
	
	// Handle axis
	// Left stick to move
	// We need the direction of the camera, normalized.
	/*if(_axes[_codes[MOVE_FORWARD]]*_axes[_codes[MOVE_FORWARD]] + _axes[_codes[MOVE_LATERAL]]*_axes[_codes[MOVE_LATERAL]] > 0.1){
		// Update the camera position.
		
	}*/
	
	// L2 and R2 triggers are used to move up and down. They can be read like axis.
	/*if(_axes[_codes[MOVE_UP]] > -0.9){
		;
	}*/
	
	
	
}

bool Joystick::configure(){
	
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
		return false;
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
	return true;

}


std::string Joystick::trim(const std::string & str, const std::string & del){
	const size_t firstNotDel = str.find_first_not_of(del);
	if(firstNotDel == std::string::npos){
		return "";
	}
	const size_t lastNotDel = str.find_last_not_of(del);
	return str.substr(firstNotDel, lastNotDel - firstNotDel + 1);
}

