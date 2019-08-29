#pragma once

#include "input/controller/Controller.hpp"
#include "Common.hpp"

/**
 \brief Represents a controller used for debug, where all raw buttons are shown.
 \ingroup Input
 */
class RawController : public Controller {

public:
	
	/// Constructor
	RawController();
	
	/**
	 \copydoc Controller::activate
	 */
	bool activate(int id) override;
	
	/**
	 \copydoc Controller::deactivate
	 */
	void deactivate() override;
	
	/**
	 \copydoc Controller::update
	 */
	void update() override;
		
	std::vector<float> allAxes; ///< Raw intensity values on all axes.
	std::vector<Controller::ControllerButton> allButtons; ///< State of each button.
	
private:
	
	int _rawAxesCount = 0; ///< Number of axes returned by GLFW
	int _rawButtonsCount = 0; ///< Number of buttons returned by GLFW
	
};
