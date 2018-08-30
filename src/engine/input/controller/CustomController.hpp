#ifndef CustomController_h
#define CustomController_h

#include "Controller.hpp"
#include "../../Common.hpp"
#include <map>

class CustomController : public Controller {

public:
	
	CustomController();

	~CustomController();
	
	bool activate(int id);
	
	void deactivate();
	
	/// Update the values for axes and buttons.
	void update();
	
private:
	
	bool configure(const std::string & mapFile);
	
	// References to GLFW flags.
	int _rawAxesCount = 0;
	int _rawButtonsCount = 0;
	const float * _rawAxes = NULL;
	const unsigned char * _rawButtons = NULL;
	
	int _buttonCodes[ControllerInput::ControllerInputCount];
	int _axisCodes[ControllerInput::ControllerInputCount];
	
};

#endif
