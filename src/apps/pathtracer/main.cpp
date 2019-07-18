
#include "BVHRenderer.hpp"
#include "scene/Scene.hpp"
#include "resources/ResourcesManager.hpp"
#include "helpers/Random.hpp"
#include "helpers/System.hpp"
#include "input/Input.hpp"
#include "Config.hpp"
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
	PathTracerConfig(const std::vector<std::string> & argv) : RenderingConfig(argv) {
		
		// Process arguments.
		for(const auto & arg : _rawArguments){
			const std::string key = arg.key;
			const std::vector<std::string> & values = arg.values;
			
			if(key == "samples" && values.size() >= 1){
				samples = size_t(std::stoi(values[0]));
			} else if(key == "depth" && values.size() >= 1){
				depth = size_t(std::stoi(values[0]));
			} else if(key == "scene" && values.size() >= 1){
				scene = values[0];
			} else if(key == "output" && values.size() >= 1){
				outputPath = values[0];
			} else if(key == "size" && values.size() >= 2){
				size[0] = std::stoi(values[0]);
				size[1] = std::stoi(values[1]);
			} else if(key == "render"){
				directRender = true;
			}
		}
		
		// Ensure that the samples count is a power of 2.
		const size_t samplesOld = samples;
		samples = std::pow(2, std::round(std::log2(float(samplesOld))));
		if(samplesOld != samples){
			Log::Warning() << "Non power-of-2 samples count. Using " << samples << " instead." << std::endl;
		}
		
		// If no path passed, setup a default one.
		if(outputPath.empty()){
			outputPath 	= "./test_" + scene + "_" + std::to_string(samples) + "_" + std::to_string(depth)
			+ "_" + std::to_string(size.x) + "x" + std::to_string(size.y) + ".png";
		}
		
		// Detail help.
		_infos.emplace_back("", "", "Path tracer");
		_infos.emplace_back("size", "", "Dimensions of the image.", std::vector<std::string>{"width", "height"});
		_infos.emplace_back("samples", "", "Number of samples per pixel (closest power of 2).", "int");
		_infos.emplace_back("depth", "", "Maximum path depth.", "int");
		_infos.emplace_back("scene", "", "Name of the scene to load.", "string");
		_infos.emplace_back("output", "", "Path for the output image.", "path");
		_infos.emplace_back("render", "", "Disable the GUI and run a render immediatly.");
		
	}

	
	glm::ivec2 size = glm::ivec2(1024); ///< Image size.
	size_t samples = 8; ///< Number of samples per pixel, should be a power of two.
	size_t depth = 5; ///< Max depth of a path.
	std::string outputPath = ""; ///< Output image path.
	std::string scene = ""; ///< Scene name.
	bool directRender = false; ///< Disable the GUI and run a render immediatly.
	
};


/** Load a scene and performs a path tracer rendering using the settings in the configuration.
 The camera used will be the scene reference viewpoint defined in the scene file.
 The output will be saved to the path specified in the configuration.
 \param config the run configuration
 \ingroup PathtracerDemo
 */
void renderOneShot(const PathTracerConfig & config){
	
	// Load geometry and create raycaster.
	std::shared_ptr<Scene> scene(new Scene(config.scene));
	// For offline renders we only need the CPU data.
	scene->init(Storage::CPU);
	
	// Create the result image.
	Image render(config.size.x, config.size.y, 3);
	// Setup camera at the proper ratio.
	Camera camera = scene->viewpoint();
	const float ratio = float(config.size.x)/float(config.size.y);
	camera.ratio(ratio);
	
	PathTracer tracer(scene);
	tracer.render(camera, config.samples, config.depth, render);
	
	// Save image.
	ImageUtilities::saveLDRImage(config.outputPath, render, false);
	
	System::ping();
}


/**
 The main function of the demo.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup PathtracerDemo
 */
int main(int argc, char** argv) {
	
	PathTracerConfig config(std::vector<std::string>(argv, argv+argc));
	if(config.showHelp()){
		return 0;
	}
	
	if(config.scene.empty()){
		Log::Error() << "Missing scene name." << std::endl;
		return 1;
	}
	
	// Initialize random generator;
	Random::seed();
	Resources::manager().addResources("../../../resources/pbrdemo");
	Resources::manager().addResources("../../../resources/additional");
	
	// Headless mode: use the scene reference camera to perform rendering immediatly and saving it to disk.
	if(config.directRender){
		renderOneShot(config);
		return 0;
	}
	
	GLFWwindow* window = System::initWindow("Path tracer", config);
	if(!window){
		return -1;
	}
	
	// Load geometry and create raycaster.
	std::shared_ptr<Scene> scene(new Scene(config.scene));
	// We need th CPU data for the path tracer, the GPU data for the preview.
	scene->init(Storage::BOTH);
	
	std::unique_ptr<BVHRenderer> renderer(new BVHRenderer(config));
	renderer->setScene(scene);
	
	double timer = glfwGetTime();
	double fullTime = 0.0;
	double remainingTime = 0.0;
	const double dt = 1.0/120.0; // Small physics timestep.
	
	// Start the display/interaction loop.
	while (!glfwWindowShouldClose(window)) {
		// Update events (inputs,...).
		Input::manager().update();
		// Handle quitting.
		if(Input::manager().pressed(Input::KeyEscape)){
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		
		// Start a new frame for the interface.
		System::GUI::beginFrame();
		
		// We separate punctual events from the main physics/movement update loop.
		renderer->update();
		
		// Compute the time elapsed since last frame
		double currentTime = glfwGetTime();
		double frameTime = currentTime - timer;
		timer = currentTime;
		
		// Physics simulation
		// First avoid super high frametime by clamping.
		if(frameTime > 0.2){ frameTime = 0.2; }
		// Accumulate new frame time.
		remainingTime += frameTime;
		// Instead of bounding at dt, we lower our requirement (1 order of magnitude).
		while(remainingTime > 0.2*dt){
			double deltaTime = fmin(remainingTime, dt);
			// Update physics and camera.
			renderer->physics(fullTime, deltaTime);
			// Update timers.
			fullTime += deltaTime;
			remainingTime -= deltaTime;
		}
		
		// Update the content of the window.
		renderer->draw();
		// Then render the interface.
		System::GUI::endFrame();
		//Display the result for the current rendering loop.
		glfwSwapBuffers(window);
		
	}
	
	// Clean the interface.
	System::GUI::clean();
	// Clean other resources
	renderer->clean();
	Resources::manager().clean();
	// Close GL context and any other GLFW resources.
	glfwDestroyWindow(window);
	glfwTerminate();
	
	return 0;
}


