#include "Controller.hpp"
#include "../../resources/ResourcesManager.hpp"
#include <sstream>



Controller::Controller() {
	reset();
} 

void Controller::reset(){
	_id = -1;
	// Reset pressed buttons.
	for(unsigned int i = 0; i < ControllerInput::ControllerInputCount; ++i){
		_buttons[i].pressed = false;
		_buttons[i].first = false;
		_axes[i] = 0.0f;
	}
}
//bool Joystick::activate(int id){
//	_id = id;
//	// Get axes and buttons references and count from GLFW
//	_rawAxes = glfwGetJoystickAxes(_id, &_rawAxesCount);
//	_rawButtons = glfwGetJoystickButtons(_id, &_rawButtonsCount);
//	for(unsigned int i = 0; i < JoystickInput::JoystickInputCount; ++i){
//		_buttons[i].pressed = false;
//		_buttons[i].first = false;
//	}
//	return configure();
//}
//
//void Joystick::deactivate(){
//	_id = -1;
//}

bool Controller::pressed(const ControllerInput & input) const {
	return _buttons[input].pressed;
}

bool Controller::triggered(const ControllerInput & input, bool absorb) {
	bool res = _buttons[input].first;
	if(absorb){
		_buttons[input].first = false;
	}
	return res;
}

float Controller::axis(const ControllerInput & input) const {
	return _axes[input];
}





