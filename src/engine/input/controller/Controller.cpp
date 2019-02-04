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

Controller::~Controller(){
	
}

void Controller::saveConfiguration(const std::string & outputPath, const std::string & guid, const std::string & name, const std::vector<int> & axesMapping, const std::vector<int> & buttonsMapping){
	
	// Build hexadecimal representation of the GUID.
	static const char* digits = "0123456789ABCDEF";
	std::string hexGUID;
	for(int i = 0; i < guid.size(); ++i){
		const unsigned char c = guid[i];
		hexGUID.push_back(digits[(c>>4) & 0x0F]);
		hexGUID.push_back(digits[c & 0x0F]);
	}
	
	// Determine the current platform.
#ifdef _WIN32
	const std::string platform = "Windows";
#elif defined(__APPLE__) && defined(__MACH__)
	const std::string platform = "Mac OS X";
#else
	const std::string platform = "Linux";
#endif
	
	std::stringstream outputStr;
	outputStr << hexGUID << "," << name << ",platform:" << platform << ",";
	
	// Write the mappings.
	static const std::vector<std::string> internalToSdlNames = {
		"c", "d", "a", "b",
		"leftshoulder", "lefttrigger", "leftstick",
		"rightshoulder", "righttrigger", "rightstick",
		"dpup", "dpleft", "dpdown", "dpright",
		"guide", "start", "back",
		"leftx", "lefty", "rightx", "righty"
	};
	
	for(int i = 0; i < Controller::ControllerInputCount; ++i){
		outputStr << internalToSdlNames[i] << ":";
		const int bid = buttonsMapping[i];
		const int aid = axesMapping[i];
		// Assume axis have a higher priority.
		if(aid >= 0){
			outputStr << "a" << aid;
		} else if(bid >= 0){
			outputStr << "b" << bid;
		}
		outputStr << ",";
	}
	Resources::saveStringToExternalFile(outputPath, outputStr.str());
}

bool Controller::parseConfiguration(const std::string & settingsContent, std::vector<int> & axesMapping, std::vector<int> & buttonsMapping){
	
	// If no mapping found, return.
	if(settingsContent.empty()){
		Log::Error() << Log::Input << "No settings found for the controller." << std::endl;
		return false;
	}
	axesMapping.clear();
	buttonsMapping.clear();
	axesMapping.resize(Controller::ControllerInputCount, -1);
	buttonsMapping.resize(Controller::ControllerInputCount, -1);
	
	static const std::map<std::string, Controller::ControllerInput> sdlNamesToInternal = {
		{ "c", Controller::ButtonX }, { "d", Controller::ButtonY }, { "a", Controller::ButtonA }, { "b", Controller::ButtonB },
		{"leftshoulder", Controller::BumperL1 }, {"lefttrigger", Controller::TriggerL2 }, {"leftstick", Controller::ButtonL3 },
		{"rightshoulder", Controller::BumperR1 }, {"righttrigger", Controller::TriggerR2 }, {"rightstick", Controller::ButtonR3 },
		{"dpup", Controller::ButtonUp }, {"dpleft", Controller::ButtonLeft }, {"dpdown", Controller::ButtonDown }, {"dpright", Controller::ButtonRight },
		{"guide", Controller::ButtonLogo }, {"start", Controller::ButtonMenu }, {"back", Controller::ButtonView },
		{"leftx", Controller::PadLeftX }, {"lefty", Controller::PadLeftY },
		{"rightx", Controller::PadRightX }, {"righty", Controller::PadRightY }
	};
	
	std::istringstream line(settingsContent);
	std::vector<std::string> tokens;
	std::string token;
	while(std::getline(line, token, ',')) {
		tokens.push_back(token);
	}
	
	// Skip the first three tokens, containing the GUID, the name and platform.
	for(int tid = 3; tid < tokens.size(); ++tid){
		const std::string & token = tokens[tid];
		const size_t separatorPos = token.find(":");
		if(separatorPos == std::string::npos){
			Log::Warning() << Log::Input << "Malformed token \"" << token << "\"." << std::endl;
			continue;
		}
		const std::string name = token.substr(0, separatorPos);
		const std::string value = token.substr(separatorPos+1);
		
		if(value.size() < 2){
			continue;
		}
		int bval = -1;
		int aval = -1;
		if(value[0] == 'b'){
			bval = std::stoi(value.substr(1));
		} else if(value[0] == 'a'){
			aval = std::stoi(value.substr(1));
		} else {
			Log::Warning() << Log::Input << "Controller configuration file contains erroneous code." << std::endl;
		}
		const auto currentButton = sdlNamesToInternal.at(name);
		buttonsMapping[currentButton] = bval;
		axesMapping[currentButton] = aval;
	}
	return true;
}


