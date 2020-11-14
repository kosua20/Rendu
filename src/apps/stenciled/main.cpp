#include "StenciledApp.hpp"
#include "generation/Random.hpp"
#include "input/Input.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "scene/Scene.hpp"
#include "Common.hpp"

/**
 \defgroup StencilDemo Stencil demo
 \brief Example of using the stencil buffer for visual effects
 \details Perform black and white rendering of a scene with alternating stripes depending on the number of primitives covering each surface.
 \ingroup Applications
 */

/**
 The main function of the physically-based rendering demo. Handles the setup and main loop.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup StencilDemo
 */
int main(int argc, char ** argv) {

	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}

	Window window("PBR demo", config, true);
	
	Resources::manager().addResources("../../../resources/common");
	Resources::manager().addResources("../../../resources/pbrdemo");
	Resources::manager().addResources("../../../resources/additional");
	if(!config.resourcesPath.empty()){
		Resources::manager().addResources(config.resourcesPath);
	}
	
	// Seed random generator.
	Random::seed();

	// Create the renderer.
	StenciledApp app(config);

	// Start the display/interaction loop.
	while(window.nextFrame()) {
		app.update();
		app.draw();
	}

	// Clean other resources
	Resources::manager().clean();

	return 0;
}
