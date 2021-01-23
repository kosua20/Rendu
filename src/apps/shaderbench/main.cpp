#include "ShaderEditor.hpp"

#include "resources/ResourcesManager.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "generation/Random.hpp"
#include "system/Config.hpp"
#include "Common.hpp"

/**
 \defgroup ShaderBench Shader bench
 \brief Shader visualisation tool, inspired by ShaderToy and Bonzomatic
 \ingroup Applications
 */

/**
 The main function of the shader editing tool.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup ShaderBench
 */
int main(int argc, char ** argv) {
	
	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}
	
	Window window("Shader Editor", config, false);

	Resources::manager().addResources("../../../resources/shaderbench");
	if(!config.resourcesPath.empty()){
		Resources::manager().addResources(config.resourcesPath);
	}

	// Seed random generator in a reproducible fashion.
	Random::seed(0x0decafe);
	
	ShaderEditor app(config);
	
	// Start the display/interaction loop.
	while(window.nextFrame()) {
		app.update();
		app.draw();
	}
	
	return 0;
}
