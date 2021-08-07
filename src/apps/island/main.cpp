#include "IslandApp.hpp"

#include "resources/ResourcesManager.hpp"
#include "generation/Random.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "system/Config.hpp"
#include "Common.hpp"

/**
 \defgroup Island Island
 \brief Render an island in the middle of the ocean
 \sa IslandApp
 \ingroup Applications
 */

/**
 The main function of the island and ocean rendering demo.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup Island
 */
int main(int argc, char ** argv) {
	
	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}
	
	Window window("Island", config);
	
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
	
	return 0;
}
