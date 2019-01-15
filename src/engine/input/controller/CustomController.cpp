#include "CustomController.hpp"
#include "../../resources/ResourcesManager.hpp"
#include <sstream>




CustomController::CustomController() : Controller() {
	_buttonCodes.resize(ControllerInput::ControllerInputCount, -1);
	_axisCodes.resize(ControllerInput::ControllerInputCount, -1);
} 

bool CustomController::activate(int id){
	reset();
	
	_id = id;
	// Get axes and buttons references and count from GLFW
	_rawAxes = glfwGetJoystickAxes(_id, &_rawAxesCount);
	_rawButtons = glfwGetJoystickButtons(_id, &_rawButtonsCount);
	
	Log::Info() << Log::Input << "Joystick named " << std::string(glfwGetJoystickName(_id)) << "." << std::endl;
	
	const std::string settingsContent = Resources::manager().getString("controller_ps4.map");
	const bool parseStatus = CustomController::parseConfiguration(settingsContent, _axisCodes, _buttonCodes);
	if(!parseStatus){
		_id = -1;
	}
	return parseStatus;
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
			const bool pressed = (_rawButtons[bcode] == GLFW_PRESS);
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
		} else if(i == TriggerL2 || i == TriggerR2){
			// Hack to support both button and axis triggers in all cases.
			_axes[i] = float(_buttons[i].pressed);
		}
	}
	
}

void CustomController::saveConfiguration(const std::string & outputPath, const std::vector<int> & axesMapping, const std::vector<int> & buttonsMapping){
	std::stringstream outputStr;
	
	// Read the first line to know the number of buttons and axes.
	std::vector<std::string> inputNames = {
		"ButtonX", "ButtonY", "ButtonA", "ButtonB", "BumperL1", "TriggerL2", "ButtonL3",
		"BumperR1", "TriggerR2", "ButtonR3", "ButtonUp", "ButtonLeft", "ButtonDown", "ButtonRight",
		"ButtonLogo", "ButtonMenu", "ButtonView", "PadLeftX", "PadLeftY", "PadRightX", "PadRightY",
	};
	for(int i = 0; i < Controller::ControllerInputCount; ++i){
		outputStr << inputNames[i] << " : ";
		const int bid = buttonsMapping[i];
		const int aid = axesMapping[i];
		if(aid >= 0){
			outputStr << "A" << aid;
		}
		if(bid >= 0){
			if(aid >= 0){
				outputStr << ",";
			}
			outputStr << "B" << bid;
		}
		outputStr << std::endl;
	}
	Resources::saveStringToExternalFile(outputPath, outputStr.str());
}

bool CustomController::parseConfiguration(const std::string & settingsContent, std::vector<int> & axesMapping, std::vector<int> & buttonsMapping){
	
	// If no mapping found, return. \todo disable cotroller
	if(settingsContent.empty()){
		Log::Error() << Log::Input << "No settings found for the controller." << std::endl;
		return false;
	}
	axesMapping.clear();
	buttonsMapping.clear();
	axesMapping.resize(Controller::ControllerInputCount, -1);
	buttonsMapping.resize(Controller::ControllerInputCount, -1);
	
	// Parse the config file and update the map with it.
	std::istringstream lines(settingsContent);
	std::string line;
	// Read the first line to know the number of buttons and axes.
	while(std::getline(lines,line)){
		// Skip comments.
		if(line.empty() || line[0] == '#'){
			continue;
		}
		// Split line at colon.
		const size_t separatorPos = line.find_first_of(":");
		// If no colon, skip.
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
			buttonsMapping[Controller::ButtonX] = bval;
			axesMapping[Controller::ButtonX] = aval;
		} else if(key == "ButtonY"){
			buttonsMapping[Controller::ButtonY] = bval;
			axesMapping[Controller::ButtonY] = aval;
		} else if(key == "ButtonA"){
			buttonsMapping[Controller::ButtonA] = bval;
			axesMapping[Controller::ButtonA] = aval;
		} else if(key == "ButtonB"){
			buttonsMapping[Controller::ButtonB] = bval;
			axesMapping[Controller::ButtonB] = aval;
		} else if(key == "BumperL1"){
			buttonsMapping[Controller::BumperL1] = bval;
			axesMapping[Controller::BumperL1] = aval;
		} else if(key == "TriggerL2"){
			buttonsMapping[Controller::TriggerL2] = bval;
			axesMapping[Controller::TriggerL2] = aval;
		} else if(key == "ButtonL3"){
			buttonsMapping[Controller::ButtonL3] = bval;
			axesMapping[Controller::ButtonL3] = aval;
		} else if(key == "BumperR1"){
			buttonsMapping[Controller::BumperR1] = bval;
			axesMapping[Controller::BumperR1] = aval;
		} else if(key == "TriggerR2"){
			buttonsMapping[Controller::TriggerR2] = bval;
			axesMapping[Controller::TriggerR2] = aval;
		} else if(key == "ButtonR3"){
			buttonsMapping[Controller::ButtonR3] = bval;
			axesMapping[Controller::ButtonR3] = aval;
		} else if(key == "ButtonUp"){
			buttonsMapping[Controller::ButtonUp] = bval;
			axesMapping[Controller::ButtonUp] = aval;
		} else if(key == "ButtonLeft"){
			buttonsMapping[Controller::ButtonLeft] = bval;
			axesMapping[Controller::ButtonLeft] = aval;
		} else if(key == "ButtonDown"){
			buttonsMapping[Controller::ButtonDown] = bval;
			axesMapping[Controller::ButtonDown] = aval;
		} else if(key == "ButtonRight"){
			buttonsMapping[Controller::ButtonRight] = bval;
			axesMapping[Controller::ButtonRight] = aval;
		} else if(key == "ButtonLogo"){
			buttonsMapping[Controller::ButtonLogo] = bval;
			axesMapping[Controller::ButtonLogo] = aval;
		} else if(key == "ButtonMenu"){
			buttonsMapping[Controller::ButtonMenu] = bval;
			axesMapping[Controller::ButtonMenu] = aval;
		} else if(key == "ButtonView"){
			buttonsMapping[Controller::ButtonView] = bval;
			axesMapping[Controller::ButtonView] = aval;
		} else if(key == "PadLeftX"){
			buttonsMapping[Controller::PadLeftX] = bval;
			axesMapping[Controller::PadLeftX] = aval;
		} else if(key == "PadLeftY"){
			buttonsMapping[Controller::PadLeftY] = bval;
			axesMapping[Controller::PadLeftY] = aval;
		} else if(key == "PadRightX"){
			buttonsMapping[Controller::PadRightX] = bval;
			axesMapping[Controller::PadRightX] = aval;
		} else if(key == "PadRightY"){
			buttonsMapping[Controller::PadRightY] = bval;
			axesMapping[Controller::PadRightY] = aval;
		} else {
			// Unknown key, ignore.
			Log::Error() << Log::Input << "Controller configuration file contains unknown key: " << key << "." << std::endl;
		}
	}
	return true;
	
}

