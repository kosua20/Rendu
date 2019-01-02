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
	
	/**
	 Parse a buttons/axes mapping configuration from the given string.
	 \param settingsContent the string containing the configuration to parse
	 \param axesMapping the axes mapping to populate
	 \param buttonsMapping the buttons mapping to populate
	 \return true if the configuration was properly parsed
	 */
	static bool parseConfiguration(const std::string & settingsContent, std::vector<int> & axesMapping, std::vector<int> & buttonsMapping);
	
	/**
	 Save a configuration to a file on disk.
	 \param outputPath the file to write the configuration to
	 \param axesMapping the axes mapping to save
	 \param buttonsMapping the buttons mapping to save
	 */
	static void saveConfiguration(const std::string & outputPath, const std::vector<int> & axesMapping, const std::vector<int> & buttonsMapping);
	
private:
	
	int _rawAxesCount = 0; ///< Number of axes returned by GLFW
	int _rawButtonsCount = 0; ///< Number of buttons returned by GLFW
	const float * _rawAxes = NULL; ///< Pointer to the axes returned by GLFW
	const unsigned char * _rawButtons = NULL; ///< Pointer to the buttons returned by GLFW
	
	std::vector<int> _buttonCodes; ///< Mapping from each ControllerInput to a raw GLFW button ID.
	std::vector<int> _axisCodes; ///< Mapping from each ControllerInput to a raw GLFW axis ID.
	
};

#endif
