#ifndef CustomController_h
#define CustomController_h

#include "Controller.hpp"
#include "../../Common.hpp"
#include <map>

/**
 \brief Represents a controller with a custom mapping loaded from disk.
 \ingroup Input
 */
class CustomController : public Controller {

public:
	
	/// Constructor
	CustomController();
	
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
	
private:
	
	/**
	 Load a buttons mapping configuration file from the given path.
	 \param mapFile the path to the mapping file
	 \return true if the controller was properly setup
	 */
	bool configure(const std::string & mapFile);
	
	
	int _rawAxesCount = 0; ///< Number of axes returned by GLFW
	int _rawButtonsCount = 0; ///< Number of buttons returned by GLFW
	const float * _rawAxes = NULL; ///< Pointer to the axes returned by GLFW
	const unsigned char * _rawButtons = NULL; ///< Pointer to the buttons returned by GLFW
	
	int _buttonCodes[ControllerInput::ControllerInputCount]; ///< Mapping from each ControllerInput to a raw GLFW button ID.
	int _axisCodes[ControllerInput::ControllerInputCount]; ///< Mapping from each ControllerInput to a raw GLFW axis ID.
	
};

#endif
