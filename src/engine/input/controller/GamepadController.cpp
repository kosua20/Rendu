#include "input/controller/GamepadController.hpp"
#include "Common.hpp"

GamepadController::GamepadController() : Controller() {
} 

bool GamepadController::activate(int id){
	if(!glfwJoystickIsGamepad(id)){
		_id = -1;
		return false;
	}
	
	reset();
	_id = id;
	_name = std::string(glfwGetGamepadName(_id));
	_guid = std::string(glfwGetJoystickGUID(_id));
	Log::Info() << Log::Input << "Gamepad named " << _name << "." << std::endl;
	return true;
}

void GamepadController::deactivate(){
	_id = -1;
}

void GamepadController::update(){
	
	GLFWgamepadstate state;
	glfwGetGamepadState(_id, &state);
	
	static const std::vector<Controller::Input> glfwButtonsToInternal = {
		Controller::Input::ButtonA, Controller::Input::ButtonB, Controller::Input::ButtonX, Controller::Input::ButtonY,
		Controller::Input::BumperL1, Controller::Input::BumperR1,
		Controller::Input::ButtonView, Controller::Input::ButtonMenu, Controller::Input::ButtonLogo,
		Controller::Input::ButtonL3, Controller::Input::ButtonR3,
		Controller::Input::ButtonUp, Controller::Input::ButtonRight, Controller::Input::ButtonDown, Controller::Input::ButtonLeft
	};
	
	static const std::vector<Controller::Input> glfwAxesToInternal = {
		Controller::Input::PadLeftX, Controller::Input::PadLeftY, Controller::Input::PadRightX, Controller::Input::PadRightY,
		Controller::Input::TriggerL2, Controller::Input::TriggerR2
	};
	
	for(unsigned int i = 0; i < glfwButtonsToInternal.size(); ++i){
		const bool pressed = state.buttons[i] == GLFW_PRESS;
		const uint button = uint(glfwButtonsToInternal[i]);
		if(pressed){
			if(_buttons[button].pressed){
				// Already pressed.
				_buttons[button].first = false;
			} else {
				_buttons[button].pressed = true;
				_buttons[button].first = true;
			}
		} else {
			_buttons[button].pressed = false;
			_buttons[button].first = false;
		}
	}
	
	for(unsigned int i = 0; i < glfwAxesToInternal.size(); ++i){
		_axes[uint(glfwAxesToInternal[i])] = state.axes[i];
	}
	
}
