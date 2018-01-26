#ifndef Joystick_h
#define Joystick_h

#include <glm/glm.hpp>
#include <map>

class Joystick {

public:
	
	enum JoystickInput {
		MoveForward, MoveLateral, LookVertical, LookHorizontal, MoveUp, MoveDown,
		ResetAll, ResetCenter, ResetOrientation, SpeedUp, SpeedDown, JoystickInputCount
	};
	
	Joystick();

	~Joystick();
	
	bool activate(int id);
	
	void deactivate();
	
	/// Update the values for axes and buttons.
	void update();
	
	/// Queries.
	bool pressed(const JoystickInput & input) const;
	
	bool triggered(const JoystickInput & input, bool absorb = false);
	
	float axis(const JoystickInput & input) const;
	
private:
	
	
	bool configure();
	
	/// Joystick ID (or -1 if no joystick is connected).
	int _id = -1;
	
	// References to GLFW flags.
	int _rawAxesCount = 0;
	int _rawButtonsCount = 0;
	const float * _rawAxes = NULL;
	const unsigned char * _rawButtons = NULL;
	
	struct JoystickButton {
		bool pressed = false;
		bool first = false;
	};
	JoystickButton _buttons[JoystickInput::JoystickInputCount];
	
	std::map<JoystickInput, int> _codes = {};
	
};

#endif
