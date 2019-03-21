#ifndef GamepadController_h
#define GamepadController_h

#include "input/controller/Controller.hpp"
#include "Common.hpp"

/**
 \brief Represents a controller with a predefined mapping provided by GLFW/SDL.
 \ingroup Input
 */
class GamepadController : public Controller {

public:
	
	/// Constructor
	GamepadController();
	
	/**
	 \copydoc Controller::activate
	 */
	bool activate(int id);
	
	/**
	 \copydoc Controller::deactivate
	 */
	void deactivate();
	
	/**
	 \copydoc Controller::update
	 */
	void update();

	
};

#endif
