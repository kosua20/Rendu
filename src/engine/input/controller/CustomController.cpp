#include "CustomController.hpp"
#include "../../resources/ResourcesManager.hpp"
#include <sstream>




CustomController::CustomController() : Controller() {
	for(unsigned int i = 0; i < ControllerInput::ControllerInputCount; ++i){
		_buttonCodes[i] = -1;
		_axisCodes[i] = -1;
	}
} 

CustomController::~CustomController(){}

bool CustomController::activate(int id){
	reset();
	
	_id = id;
	// Get axes and buttons references and count from GLFW
	_rawAxes = glfwGetJoystickAxes(_id, &_rawAxesCount);
	_rawButtons = glfwGetJoystickButtons(_id, &_rawButtonsCount);
	
	Log::Info() << Log::Input << "Joystick named " << std::string(glfwGetJoystickName(_id)) << "." << std::endl;
	return configure("controller_ps4.map");
}

void CustomController::deactivate(){
	_id = -1;
}

void CustomController::update(){
	// Update buttons flags.
	_rawAxes = glfwGetJoystickAxes(_id, &_rawAxesCount);
	_rawButtons = glfwGetJoystickButtons(_id, &_rawButtonsCount);
	
	// Translate from raw buttons to clean buttons.
	for(unsigned int i = 0; i < ControllerInput::ControllerInputCount; ++i){
		const int bcode = _buttonCodes[i];
		if(bcode >= 0){
			bool pressed = (_rawButtons[bcode] == GLFW_PRESS);
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
		int acode = _axisCodes[i];
		if(acode >= 0){
		_axes[i] = _rawAxes[acode];
		}
	}
	
}


bool CustomController::configure(const std::string & mapFile){
	
	const std::string settingsContent = Resources::manager().getString(mapFile);
	// If no mapping found, disable the controller.
	if(settingsContent.empty()){
		Log::Error() << Log::Input << "No settings found for the controller." << std::endl;
		_id = -1;
		return false;
	}
	// Parse the config file and update the map with it.
	for(unsigned int i = 0; i < ControllerInput::ControllerInputCount; ++i){
		_buttonCodes[i] = -1;
		_axisCodes[i] = -1;
	}
	
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
		
		int bval = -1;
		int aval = -1;
		
		const std::string codes = Resources::trim(Resources::trim(line.substr(separatorPos + 1), " "), "\t");
		const size_t separator2Pos = codes.find_first_of(",");
		std::vector<std::string> codeStrings;
		if(separator2Pos == std::string::npos){
			codeStrings.push_back(codes);
		} else {
			// Split.
			codeStrings.push_back(codes.substr(0, separator2Pos));
			codeStrings.push_back(codes.substr(separator2Pos+1));
		}
		
		for(const auto & codeStr : codeStrings){
			if(codeStr.size() < 2){
				continue;
			}
			if(codeStr[0] == 'B'){
				bval = std::stoi(codeStr.substr(1));
			} else if(codeStr[0] == 'A'){
				aval = std::stoi(codeStr.substr(1));
			} else {
				Log::Warning() << Log::Input << "Controller configuration file contains erroneous code." << std::endl;
			}
		}
		
		if(key == "ButtonX") {
			_buttonCodes[ButtonX] = bval;
			_axisCodes[ButtonX] = aval;
		} else if(key == "ButtonY"){
			_buttonCodes[ButtonY] = bval;
			_axisCodes[ButtonY] = aval;
		} else if(key == "ButtonA"){
			_buttonCodes[ButtonA] = bval;
			_axisCodes[ButtonA] = aval;
		} else if(key == "ButtonB"){
			_buttonCodes[ButtonB] = bval;
			_axisCodes[ButtonB] = aval;
		} else if(key == "BumperL1"){
			_buttonCodes[BumperL1] = bval;
			_axisCodes[BumperL1] = aval;
		} else if(key == "TriggerL2"){
			_buttonCodes[TriggerL2] = bval;
			_axisCodes[TriggerL2] = aval;
		} else if(key == "ButtonL3"){
			_buttonCodes[ButtonL3] = bval;
			_axisCodes[ButtonL3] = aval;
		} else if(key == "BumperR1"){
			_buttonCodes[BumperR1] = bval;
			_axisCodes[BumperR1] = aval;
		} else if(key == "TriggerR2"){
			_buttonCodes[TriggerR2] = bval;
			_axisCodes[TriggerR2] = aval;
		} else if(key == "ButtonR3"){
			_buttonCodes[ButtonR3] = bval;
			_axisCodes[ButtonR3] = aval;
		} else if(key == "ButtonUp"){
			_buttonCodes[ButtonUp] = bval;
			_axisCodes[ButtonUp] = aval;
		} else if(key == "ButtonLeft"){
			_buttonCodes[ButtonLeft] = bval;
			_axisCodes[ButtonLeft] = aval;
		} else if(key == "ButtonDown"){
			_buttonCodes[ButtonDown] = bval;
			_axisCodes[ButtonDown] = aval;
		} else if(key == "ButtonRight"){
			_buttonCodes[ButtonRight] = bval;
			_axisCodes[ButtonRight] = aval;
		} else if(key == "ButtonLogo"){
			_buttonCodes[ButtonLogo] = bval;
			_axisCodes[ButtonLogo] = aval;
		} else if(key == "ButtonMenu"){
			_buttonCodes[ButtonMenu] = bval;
			_axisCodes[ButtonMenu] = aval;
		} else if(key == "ButtonView"){
			_buttonCodes[ButtonView] = bval;
			_axisCodes[ButtonView] = aval;
		} else if(key == "PadLeftX"){
			_buttonCodes[PadLeftX] = bval;
			_axisCodes[PadLeftX] = aval;
		} else if(key == "PadLeftY"){
			_buttonCodes[PadLeftY] = bval;
			_axisCodes[PadLeftY] = aval;
		} else if(key == "PadRightX"){
			_buttonCodes[PadRightX] = bval;
			_axisCodes[PadRightX] = aval;
		} else if(key == "PadRightY"){
			_buttonCodes[PadRightY] = bval;
			_axisCodes[PadRightY] = aval;
		} else {
			// Unknown key, ignore.
			Log::Error() << Log::Input << "Controller configuration file contains unknown key: " << key << "." << std::endl;
		}
	}
	
	return true;

}




