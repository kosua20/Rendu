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
 \brief Shader editor configuration.
 \ingroup ShaderBench
 */
class ShaderEditorConfig : public RenderingConfig {
public:

	/** \copydoc RenderingConfig::RenderingConfig */
	explicit ShaderEditorConfig(const std::vector<std::string> & argv) :
		RenderingConfig(argv) {

		// Process arguments.
		for(const auto & arg : arguments()) {
			const std::string key					= arg.key;
			const std::vector<std::string> & values = arg.values;

			if(key == "shader" && !values.empty()) {
				shaderPath = values[0];
			}
		}

		// Detail help.
		registerSection("Shader editor");
		registerArgument("shader", "", "Path to the initial shader", "string");
	}

	std::string shaderPath; ///< Path to the initial shader to load

};

/**
 The main function of the shader editing tool.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup ShaderBench
 */
int main(int argc, char ** argv) {
	
	// First, init/parse/load configuration.
	ShaderEditorConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}

	if(!config.resourcesPath.empty()){
		Resources::manager().addResources(config.resourcesPath);
	}
	
	Window window("Shader Editor", config);

	Resources::manager().addResources("../../../resources/shaderbench");

	// Seed random generator in a reproducible fashion.
	Random::seed(0x0decafe);
	
	ShaderEditor app(config);

	// Load shader if specified.
	if(!config.shaderPath.empty()){
		app.loadShader(config.shaderPath);
	}
	
	// Start the display/interaction loop.
	while(window.nextFrame()) {
		app.update();
		app.draw();
		app.finish();
	}
	
	return 0;
}
