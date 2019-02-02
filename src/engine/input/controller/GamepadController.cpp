#include "GamepadController.hpp"
#include "../../resources/ResourcesManager.hpp"


GamepadController::GamepadController() : Controller() {
} 

bool GamepadController::activate(int id){
	if(!glfwJoystickIsGamepad(id)){
		_id = -1;
		return false;
	}
	
	reset();
	_id = id;
	Log::Info() << Log::Input << "Gamepad named " << std::string(glfwGetGamepadName(_id)) << "." << std::endl;
	return true;
}

void GamepadController::deactivate(){
	_id = -1;
}

void GamepadController::update(){
	
	GLFWgamepadstate state;
	glfwGetGamepadState(_id, &state);
	
	static const std::vector<ControllerInput> glfwButtonsToInternal = {
		ButtonA, ButtonB, ButtonX, ButtonY,
		BumperL1, BumperR1,
		ButtonView, ButtonMenu, ButtonLogo,
		ButtonL3, ButtonR3,
		ButtonUp, ButtonRight, ButtonDown, ButtonLeft
	};
	
	static const std::vector<ControllerInput> glfwAxesToInternal = {
		PadLeftX, PadLeftY, PadRightX, PadRightY,
		TriggerL2, TriggerR2
	};
	
	for(unsigned int i = 0; i < glfwButtonsToInternal.size(); ++i){
		const bool pressed = state.buttons[i] == GLFW_PRESS;
		const ControllerInput button = glfwButtonsToInternal[i];
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
		_axes[glfwAxesToInternal[i]] = state.axes[i];
	}
	
}
