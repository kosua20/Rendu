#include "FilteringApp.hpp"

#include "input/Input.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "system/Random.hpp"
#include "system/Config.hpp"
#include "Common.hpp"

/**
 \defgroup ImageFiltering Image Filtering
 \brief Apply a variety of image space filters and processings to an image.
 \details See the description of FilteringRenderer for the available filters.
 \ingroup Applications
 */

/**
 The main loop of the filtering app.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup ImageFiltering
 */
int main(int argc, char ** argv) {

	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}

	Window window("Image filtering", config);

	Resources::manager().addResources("../../../resources/common");
	Resources::manager().addResources("../../../resources/imagefiltering");

	// Seed random generator.
	Random::seed();
	
	FilteringApp app(config);

	// Start the display/interaction loop.
	while(window.nextFrame()) {
		// Reload resources.
		if(Input::manager().triggered(Input::Key::P)) {
			Resources::manager().reload();
		}
		app.update();
		app.draw();
	}
	
	// Clean resources.
	app.clean();
	Resources::manager().clean();
	window.clean();

	return 0;
}
