#include "PathTracerApp.hpp"
#include "scene/Scene.hpp"
#include "resources/ResourcesManager.hpp"
#include "generation/Random.hpp"
#include "system/System.hpp"
#include "system/Window.hpp"
#include "system/Config.hpp"
#include "input/Input.hpp"
#include "Common.hpp"

/**
 \defgroup PathtracerDemo Path tracer
 \brief A basic diffuse path tracing demo, with an interactive viewer to place the camera.
 \ingroup Applications
 */

/**
 \brief Path tracer demo configuration. Parameters for offline rendering.
 \ingroup PathtracerDemo
 */
class PathTracerConfig : public RenderingConfig {
public:
	/** \copydoc RenderingConfig::RenderingConfig */
	explicit PathTracerConfig(const std::vector<std::string> & argv) :
		RenderingConfig(argv) {

		// Process arguments.
		for(const auto & arg : arguments()) {
			const std::string key					= arg.key;
			const std::vector<std::string> & values = arg.values;

			if(key == "samples" && !values.empty()) {
				samples = size_t(std::stoi(values[0]));
			} else if(key == "depth" && !values.empty()) {
				depth = size_t(std::stoi(values[0]));
			} else if(key == "scene" && !values.empty()) {
				scene = values[0];
			} else if(key == "output" && !values.empty()) {
				outputPath = values[0];
			} else if(key == "size" && values.size() >= 2) {
				size[0] = std::stoi(values[0]);
				size[1] = std::stoi(values[1]);
			} else if(key == "render") {
				directRender = true;
			}
		}

		// Ensure that the samples count is a power of 2.
		const size_t samplesOld = samples;
		samples					= size_t(std::pow(2, std::round(std::log2(float(samplesOld)))));
		if(samplesOld != samples) {
			Log::Warning() << "Non power-of-2 samples count. Using " << samples << " instead." << std::endl;
		}

		// If no path passed, setup a default one.
		if(outputPath.empty()) {
			outputPath = "./test_" + scene + "_" + std::to_string(samples) + "_" + std::to_string(depth)
						 + "_" + std::to_string(size.x) + "x" + std::to_string(size.y) + "_" + System::timestamp() + ".png";
		}

		// Detail help.
		registerSection("Path tracer");
		registerArgument("size", "", "Dimensions of the image.", std::vector<std::string> {"width", "height"});
		registerArgument("samples", "", "Number of samples per pixel (closest power of 2).", "int");
		registerArgument("depth", "", "Maximum path depth.", "int");
		registerArgument("scene", "", "Name of the scene to load.", "string");
		registerArgument("output", "", "Path for the output image.", "path");
		registerArgument("render", "", "Disable the GUI and run a render immediatly.");
	}

	glm::ivec2 size		   = glm::ivec2(1024); ///< Image size.
	size_t samples		   = 8;				   ///< Number of samples per pixel, should be a power of two.
	size_t depth		   = 5;				   ///< Max depth of a path.
	std::string outputPath = "";			   ///< Output image path.
	std::string scene	  = "";			   	   ///< Scene name.
	bool directRender	  = false;			   ///< Disable the GUI and run a render immediatly.
};

/** Load a scene and performs a path tracer rendering using the settings in the configuration.
 The camera used will be the scene reference viewpoint defined in the scene file.
 The output will be saved to the path specified in the configuration.
 \param config the run configuration
 \ingroup PathtracerDemo
 */
void renderOneShot(const PathTracerConfig & config) {
	Resources::manager().addResources("../../../resources/common");
	// Load geometry and create raycaster.
	std::shared_ptr<Scene> scene(new Scene(config.scene));
	// For offline renders we only need the CPU data.
	if(!scene->init(Storage::CPU | Storage::FORCE_FRAME)) {
		return;
	}

	// Create the result image.
	Image render(config.size.x, config.size.y, 3);
	// Setup camera at the proper ratio.
	Camera camera	 = scene->viewpoint();
	const float ratio = float(config.size.x) / float(config.size.y);
	camera.ratio(ratio);

	PathTracer tracer(scene);

	Log::Info() << "[PathTracer] Rendering..." << std::endl;
	tracer.render(camera, config.samples, config.depth, render);

	// Save image.
	Log::Info() << "[PathTracer] Saving to " << config.outputPath << "." << std::endl;
	// Tonemap the image if needed.
	if(!Image::isFloat(config.outputPath)){
		System::forParallel(0, render.height, [&render](size_t y){
			for(uint x = 0; x < render.width; ++x){
				const glm::vec3 & color = render.rgb(int(x), int(y));
				render.rgb(int(x), int(y)) = glm::vec3(1.0f) - glm::exp(-color * 1.f);
			}
		});
	}

	// Convert to sRGB if saving in PNG.
	render.save(config.outputPath, Image::Save::SRGB_LDR | Image::Save::IGNORE_ALPHA);

	System::ping();
}

/**
 The main function of the demo.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup PathtracerDemo
 */
int main(int argc, char ** argv) {

	PathTracerConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}

	if(config.scene.empty()) {
		Log::Error() << "Missing scene name." << std::endl;
		return 1;
	}

	// Seed random generator.
	Random::seed();

	Resources::manager().addResources("../../../resources/pbrdemo");
	Resources::manager().addResources("../../../resources/additional");
	if(!config.resourcesPath.empty()){
		Resources::manager().addResources(config.resourcesPath);
	}
	
	// Headless mode: use the scene reference camera to perform rendering immediatly and saving it to disk.
	if(config.directRender) {
		renderOneShot(config);
		return 0;
	}

	Window window("Path tracer", config);
	
	// Load geometry and create raycaster.
	std::shared_ptr<Scene> scene(new Scene(config.scene));
	// We need the CPU data for the path tracer, the GPU data for the preview.
	if(!scene->init(Storage::BOTH | Storage::FORCE_FRAME)){
		return 1;
	}

	PathTracerApp app(config, scene);

	// Start the display/interaction loop.
	while(window.nextFrame()) {
		app.update();
		app.draw();
		app.finish();
	}

	return 0;
}
