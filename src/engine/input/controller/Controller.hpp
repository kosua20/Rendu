#ifndef Controller_h
#define Controller_h

#include "../../Common.hpp"
#include <map>

/**
 \brief Represents a joystick or any additional controller.
 \ingroup Input
 */
class Controller {

public:
	
	/// Constructor
	Controller();
	
	/// Controller inputs, based on the Xbox controller layout.
	enum ControllerInput {
		ButtonX, ButtonY, ButtonA, ButtonB,
		BumperL1, TriggerL2, ButtonL3,
		BumperR1, TriggerR2, ButtonR3,
		ButtonUp, ButtonLeft, ButtonDown, ButtonRight,
		ButtonLogo, ButtonMenu, ButtonView,
		PadLeftX, PadLeftY, PadRightX, PadRightY,
		ControllerInputCount
	};
	
	/**
	 Enable the controller.
	 \param id the GLFW ID of the controller.
	 \return true if the controller was correctly setup
	 */
	virtual bool activate(int id) = 0;
	
	/**
	 Disable the controller.
	 */
	virtual void deactivate() = 0;
	
	/**
	 Update the internal controller state (once per frame).
	 */
	virtual void update() = 0;
	
	/**
	 Query if a given button is currently held.
	 \param input the button
	 \return true if the button is pressed
	 */
	bool pressed(const ControllerInput & input) const;
	
	/**
	 Query if a given button was pressed at this frame precisely.
	 \param input the button
	 \param absorb should the press event be hidden from future queries during the current frame
	 \return true if the button was triggered at this frame.
	 */
	bool triggered(const ControllerInput & input, bool absorb = false);
	
	/**
	 Query the amount of displacement along a given axis (for joysticks and triggers).
	 \param input the button or pad
	 \return the current amount of displacement
	 */
	float axis(const ControllerInput & input) const;
	
	
protected:
	
	/**
	 Reset the controller state and mark it as disconnected.
	 */
	void reset();
	
	/// The state of a controller button.
	struct ControllerButton {
		bool pressed = false; ///< Is the button currently held.
		bool first = false; ///< Is it the first frame it is held.
	};
	
	ControllerButton _buttons[ControllerInput::ControllerInputCount]; ///< States of all possible buttons.
	float _axes[ControllerInput::ControllerInputCount]; ///< States of all possible axis.
	
	int _id = -1;///< Joystick ID (or -1 if no joystick is connected).
	
	
};

#endif
