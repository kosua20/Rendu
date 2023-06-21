
#include "AtmosphereApp.hpp"
#include "resources/ResourcesManager.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "generation/Random.hpp"
#include "system/Config.hpp"
#include "Common.hpp"

/**
 \defgroup AtmosphericScattering Atmospheric scattering
 \brief Demonstrate real-time approximate atmospheric scattering simulation.
 \see GPU::Frag::Atmosphere
 \ingroup Applications
 */


/**
 \brief Atmospheric scattering configuration. Parameters for precomputation.
 \ingroup AtmosphericScattering
 */
class AtmosphereConfig : public RenderingConfig {
public:
	/** Initialize a new config object, parsing the input arguments and filling the attributes with their values.
	 	\param argv the raw input arguments
	 */
	explicit AtmosphereConfig(const std::vector<std::string> & argv) :
		RenderingConfig(argv) {
		for(const auto & arg : arguments()) {
			const std::string key					= arg.key;
			const std::vector<std::string> & values = arg.values;

			if(key == "output" && !values.empty()) {
				outputPath = values[0];
			} else if(key == "samples" && !values.empty()) {
				samples = std::stoi(values[0]);
			} else if(key == "resolution" && !values.empty()) {
				resolution = size_t(std::stoi(values[0]));
			}
		}

		registerSection("Atmospheric scattering");
		registerArgument("output", "", "Output lookup table path (if specified, will only precompute and save the table).", "path/to/output.exr");
		registerArgument("samples", "", "Number of samples per-pixel.", "count");
		registerArgument("resolution", "", "Output image side size.", "size");
	}

	std::string outputPath = ""; ///< Lookup table output path.

	unsigned int samples = 256; ///< Number of samples for iterative sampling.

	size_t resolution = 512; ///< Output image resolution.
};


/**
 The main function of the atmospheric scattering demo.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup AtmosphericScattering
 */
int main(int argc, char ** argv) {
	
	// First, init/parse/load configuration.
	AtmosphereConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}

	// If an output path has been specified, precompute the table and save
	if(!config.outputPath.empty()){

		Log::Info() << Log::Utilities << "Generating scattering lookup table." << std::endl;
		// Default Earth-like atmosphere.
		const auto params = Sky::AtmosphereParameters();
		Image transmittanceTable(int(config.resolution), int(config.resolution), 4);
		AtmosphereApp::precomputeTable(params, config.samples, transmittanceTable);
		transmittanceTable.save(config.outputPath, Image::Save::NONE);

		Log::Info() << Log::Utilities << "Done." << std::endl;
		return 0;
	}

	Window window("Atmosphere", config);
	
	Resources::manager().addResources("../../../resources/atmosphere");
	if(!config.resourcesPath.empty()){
		Resources::manager().addResources(config.resourcesPath);
	}
	
	// Seed random generator.
	Random::seed();
	
	AtmosphereApp app(config, window);
	
	// Start the display/interaction loop.
	while(window.nextFrame()) {
		app.update();
		app.draw();
		app.finish();
	}
	
	return 0;
}
