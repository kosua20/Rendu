#ifndef Controller_h
#define Controller_h

#include <glm/glm.hpp>
#include <map>

class Controller {

public:
	
	Controller();
	
	enum ControllerInput {
		ButtonX, ButtonY, ButtonA, ButtonB,
		BumperL1, TriggerL2, ButtonL3,
		BumperR1, TriggerR2, ButtonR3,
		ButtonUp, ButtonLeft, ButtonDown, ButtonRight,
		ButtonLogo, ButtonMenu, ButtonView,
		PadLeftX, PadLeftY, PadRightX, PadRightY,
		ControllerInputCount
	};
	
	virtual bool activate(int id) = 0;
	
	virtual void deactivate() = 0;
	
	virtual void update() = 0;
	
	/// Queries.
	bool pressed(const ControllerInput & input) const;
	
	bool triggered(const ControllerInput & input, bool absorb = false);
	
	float axis(const ControllerInput & input) const;
	
	
protected:
	
	void reset();
	
	struct ControllerButton {
		bool pressed = false;
		bool first = false;
	};
	
	ControllerButton _buttons[ControllerInput::ControllerInputCount];
	float _axes[ControllerInput::ControllerInputCount];
	
	/// Joystick ID (or -1 if no joystick is connected).
	int _id = -1;
	
	
};

#endif
