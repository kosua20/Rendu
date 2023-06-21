#include "SceneEditor.hpp"
#include "resources/ResourcesManager.hpp"
#include "scene/Scene.hpp"
#include "input/Input.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "generation/Random.hpp"
#include "Common.hpp"

/**
 \defgroup SceneEditor Scene editor
 \brief Edit scenes and export them.
 \ingroup Tools
 */

/**
 Main loop for the scene editor.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup SceneEditor
 */
int main(int argc, char ** argv) {
	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}
	config.initialWidth = 1280;
	config.initialHeight = 700;
	Window window("Scene Editor", config);
	// Lad commons and existing scenes.
	// For now we only support editing/adding objects that are already in the resource directories.
	Resources::manager().addResources("../../../resources/pbrdemo");
	Resources::manager().addResources("../../../resources/additional");
	
	// Seed random generator.
	Random::seed();
	
	// Create the renderer.
	SceneEditor app(config, window);
	
	
	// Start the display/interaction loop.
	while(window.nextFrame()) {
		app.update();
		app.draw();
		app.finish();
	}

	return 0;
}
