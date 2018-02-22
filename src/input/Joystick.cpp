#include "Joystick.hpp"
#include "../resources/ResourcesManager.hpp"
#include "../helpers/Logger.hpp"

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>




Joystick::Joystick() {
	_id = -1;
	// Reset pressed buttons.
	for(unsigned int i = 0; i < JoystickInput::JoystickInputCount; ++i){
		_buttons[i].pressed = false;
		_buttons[i].first = false;
	}
} 

Joystick::~Joystick(){}

bool Joystick::activate(int id){
	_id = id;
	// Get axes and buttons references and count from GLFW
	_rawAxes = glfwGetJoystickAxes(_id, &_rawAxesCount);
	_rawButtons = glfwGetJoystickButtons(_id, &_rawButtonsCount);
	for(unsigned int i = 0; i < JoystickInput::JoystickInputCount; ++i){
		_buttons[i].pressed = false;
		_buttons[i].first = false;
	}
	return configure();
}

void Joystick::deactivate(){
	_id = -1;
}

void Joystick::update(){
	// Update buttons flags.
	_rawAxes = glfwGetJoystickAxes(_id, &_rawAxesCount);
	_rawButtons = glfwGetJoystickButtons(_id, &_rawButtonsCount);
	
	// Translate from raw buttons to clean buttons.
	for(unsigned int i = 0; i < JoystickInput::JoystickInputCount; ++i){
		bool pressed = (_rawButtons[_codes.at(JoystickInput(i))] == GLFW_PRESS);
		if(pressed){
			if(_buttons[i].pressed){
				// Already pressed.
				_buttons[i].first = false;
			} else {
				_buttons[i].pressed = true;
				_buttons[i].first = true;
			}
		} else {
			_buttons[i].pressed = false;
			_buttons[i].first = false;
		}
	}
	
	
}

bool Joystick::pressed(const JoystickInput & input) const {
	return _buttons[input].pressed;
}

bool Joystick::triggered(const JoystickInput & input, bool absorb) {
	bool res = _buttons[input].first;
	if(absorb){
		_buttons[input].first = false;
	}
	return res;
}

float Joystick::axis(const JoystickInput & input) const {
	return _rawAxes[_codes.at(input)];
}

bool Joystick::configure(){
	
	// Register all keys by hand for now.
	// PS4 codes: from 0 to 5
	// L horizontal, L vertical, R horizontal, R vertical, L2, R2
	// PS4 codes: from 0 to 17
	// Square, Cross, Circle, Triangle, L1, R1, L2, R2, Share, Option, L3, R3, PS button, Touchpad, Up, Right, Down, Left
	const std::string settingsContent = Resources::loadStringFromExternalFile("Controller.map");
	// If no mapping found, disable the controller.
	if(settingsContent.empty()){
		Log::Error() << Log::Input << "No settings found for the controller." << std::endl;
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
		const std::string key = Resources::trim(Resources::trim(line.substr(0, separatorPos), " "), "\t");
		const int val = std::stoi(line.substr(separatorPos + 1));
		if(key == "MOVE_FORWARD") {
			_codes[MoveForward] = val;
		} else if(key == "MOVE_LATERAL") {
			_codes[MoveLateral] = val;
		} else if(key == "LOOK_VERTICAL") {
			_codes[LookVertical] = val;
		} else if(key == "LOOK_HORIZONTAL") {
			_codes[LookHorizontal] = val;
		} else if(key == "MOVE_UP") {
			_codes[MoveUp] = val;
		} else if(key == "MOVE_DOWN") {
			_codes[MoveDown] = val;
		} else if(key == "RESET_ALL") {
			_codes[ResetAll] = val;
		} else if(key == "RESET_CENTER") {
			_codes[ResetCenter] = val;
		} else if(key == "RESET_ORIENTATION") {
			_codes[ResetOrientation] = val;
		} else if(key == "SPEED_UP") {
			_codes[SpeedUp] = val;
		} else if(key == "SPEED_DOWN") {
			_codes[SpeedDown] = val;
		} else {
			// Unknown key, ignore.
			Log::Error() << Log::Input << "Joystick configuration file contains unknown key: " << key << "." << std::endl;
		}
	}
	return true;

}




