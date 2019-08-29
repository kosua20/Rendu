#include "FilteringRenderer.hpp"

#include "input/Input.hpp"
#include "system/System.hpp"
#include "system/Random.hpp"
#include "system/Config.hpp"
#include "Common.hpp"

/**
 \defgroup ImageFiltering Image Filtering
 \brief Apply a variety of image space filters and processings to an image.
 \details See the description of FilteringRenderer for the available filters.
 \ingroup Applications
 */


/**
 The main loop of the filtering app.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup ImageFiltering
 */
int main(int argc, char** argv) {
	
	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv+argc));
	if(config.showHelp()){
		return 0;
	}
	
	GLFWwindow* window = System::initWindow("Image filtering", config);
	if(!window){
		return -1;
	}

	Resources::manager().addResources("../../../resources/common");
	Resources::manager().addResources("../../../resources/imagefiltering");
	
	// Seed random generator.
	Random::seed();
	
	// Setup the timer.
	double timer = glfwGetTime();
	double fullTime = 0.0;
	double remainingTime = 0.0;
	const double dt = 1.0/120.0; // Small physics timestep.
	
	FilteringRenderer renderer(config);
	
	// Start the display/interaction loop.
	while (!glfwWindowShouldClose(window)) {
		// Update events (inputs,...).
		Input::manager().update();
		// Handle quitting.
		if(Input::manager().pressed(Input::KeyEscape)){
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		// Reload resources.
		if(Input::manager().triggered(Input::KeyP)){
			Resources::manager().reload();
		}
		
		// Start a new frame for the interface.
		System::Gui::beginFrame();
		
		// We separate punctual events from the main physics/movement update loop.
		renderer.update();
		
		// Compute the time elapsed since last frame
		const double currentTime = glfwGetTime();
		double frameTime = currentTime - timer;
		timer = currentTime;
	
		// Physics simulation
		// First avoid super high frametime by clamping.
		if(frameTime > 0.2){ frameTime = 0.2; }
		// Accumulate new frame time.
		remainingTime += frameTime;
		// Instead of bounding at dt, we lower our requirement (1 order of magnitude).
		while(remainingTime > 0.2*dt){
			const double deltaTime = std::min(remainingTime, dt);
			// Update physics and camera.
			renderer.physics(fullTime, frameTime);
			// Update timers.
			fullTime += deltaTime;
			remainingTime -= deltaTime;
		}
		
		// Render.
		renderer.draw();
		
		// Then render the interface.
		System::Gui::endFrame();
		//Display the result for the current rendering loop.
		glfwSwapBuffers(window);
		
	}
	
	renderer.clean();
	
	// Clean the interface.
	System::Gui::clean();
	
	Resources::manager().clean();
	// Close GL context and any other GLFW resources.
	glfwDestroyWindow(window);
	glfwTerminate();
	
	return 0;
}


