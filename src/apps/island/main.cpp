#include "IslandApp.hpp"

#include "resources/ResourcesManager.hpp"
#include "generation/Random.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "system/Config.hpp"
#include "Common.hpp"

/**
 \defgroup Island Island
 \brief
 \ingroup Applications
 */

/**
 The main function of the atmospheric scattering demo.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup AtmosphericScattering
 */
int main(int argc, char ** argv) {
	
	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}
	
	Window window("Island", config);
	
	Resources::manager().addResources("../../../resources/common");
	Resources::manager().addResources("../../../resources/island");
	if(!config.resourcesPath.empty()){
		Resources::manager().addResources(config.resourcesPath);
	}
	
	// Seed random generator.
	Random::seed(8429);

	IslandApp app(config);
	
	// Start the display/interaction loop.
	while(window.nextFrame()) {
		app.update();
		app.draw();
	}
	
	// Cleaning.
	app.clean();
	Resources::manager().clean();
	window.clean();
	
	return 0;
}
