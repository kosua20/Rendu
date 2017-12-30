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
	
	bool triggered(const JoystickInput & input) const;
	
	float axis(const JoystickInput & input) const;
	
private:
	
	
	bool configure();
	
	/// Joystick ID (or -1 if no joystick is connected).
	int _id;
	
	// References to GLFW flags.
	int _rawAxesCount;
	int _rawButtonsCount;
	const float * _rawAxes;
	const unsigned char * _rawButtons;
	
	struct JoystickButton {
		bool pressed;
		bool first;
	};
	JoystickButton _buttons[JoystickInput::JoystickInputCount];
	
	std::map<JoystickInput, int> _codes;
	
};

#endif
