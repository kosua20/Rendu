#include "PBRDemo.hpp"
#include "generation/Random.hpp"
#include "input/Input.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "scene/Scene.hpp"
#include "Common.hpp"

/**
 \defgroup PBRDemo Physically-based rendering
 \brief Real-time rendering using GGX BRDF, image-based lighting, AO, tonemapped HDR, antialiasing.
 \details Two renderers can be used: a forward renderer where objects are directly shaded and the resulting colors stored, and deferred, where all scene information (such as albedo, normals, material ID, roughness,...) is rendered to a G-Buffer before being used to render each light and ambient probe contribution using simple geometric proxies.
 \sa DeferredRenderer, ForwardRenderer, Material
 \ingroup Applications
 */

/**
 The main function of the physically-based rendering demo. Handles the setup and main loop.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup PBRDemo
 */
int main(int argc, char ** argv) {

	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}

	Window window("PBR demo", config);
	
	Resources::manager().addResources("../../../resources/pbrdemo");
	Resources::manager().addResources("../../../resources/additional");
	if(!config.resourcesPath.empty()){
		Resources::manager().addResources(config.resourcesPath);
	}
	
	// Seed random generator.
	Random::seed();

	// Create the renderer.
	PBRDemo app(config, window);

	// Start the display/interaction loop.
	while(window.nextFrame()) {
		app.update();
		app.draw();
		app.finish();
	}

	return 0;
}
