#include "SceneEditor.hpp"
#include "resources/ResourcesManager.hpp"
#include "scene/Scene.hpp"
#include "input/Input.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "system/Random.hpp"
#include "Common.hpp"
#include <map>

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
	Resources::manager().addResources("../../../resources/common");
	Resources::manager().addResources("../../../resources/pbrdemo");
	Resources::manager().addResources("../../../resources/additional");
	
	// Seed random generator.
	Random::seed();
	
	// Create the renderer.
	SceneEditor app(config);
	
	
	// Start the display/interaction loop.
	while(window.nextFrame()) {
		
		// Reload resources.
		if(Input::manager().triggered(Input::Key::P)) {
			Resources::manager().reload();
		}
		app.update();
		app.draw();
	}
	
	// Clean other resources
	app.clean();
	Resources::manager().clean();
	
	window.clean();

	return 0;
}
