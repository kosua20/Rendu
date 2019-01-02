#ifndef DebugController_h
#define DebugController_h

#include "Controller.hpp"
#include "../../Common.hpp"
#include <map>

/**
 \brief Represents a controller used for debug, where all raw buttons are shown.
 \ingroup Input
 */
class DebugController : public Controller {

public:
	
	/// Constructor
	DebugController();
	
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
	
	std::string name() const { return _name; }
	int id() const { return _id; }
	
	std::vector<float> allAxes;
	std::vector<Controller::ControllerButton> allButtons;
	
private:
	std::string _name; ///< Name of the joystick
	int _rawAxesCount = 0; ///< Number of axes returned by GLFW
	int _rawButtonsCount = 0; ///< Number of buttons returned by GLFW
//	const float * _rawAxes = NULL; ///< Pointer to the axes returned by GLFW
//	const unsigned char * _rawButtons = NULL; ///< Pointer to the buttons returned by GLFW
	
	
};

#endif
