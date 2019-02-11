
#include "Common.hpp"
#include "helpers/GenerationUtilities.hpp"
#include "input/Input.hpp"
#include "helpers/InterfaceUtilities.hpp"
#include "resources/ResourcesManager.hpp"
#include "GameRenderer.hpp"
#include "GameConfig.hpp"


/**
 \defgroup SnakeGame A small 3D game demo
 \brief A small 3D game demo.
 \ingroup Applications
 */

/**
 The main function of the game demo. Handles the setup and main loop.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup SnakeGame
 */
int main(int argc, char** argv) {
	
	// First, init/parse/load configuration.
	RenderingConfig config(argc, argv);
	config.forceAspectRatio = true;
	
	GLFWwindow* window = Interface::initWindow("SnakeGame", config);
	if(!window){
		return -1;
	}
	
	Resources::manager().addResources("../../../resources/snakegame");
	// Initialize random generator;
	Random::seed();
	
	// Create the renderer.
	std::shared_ptr<GameRenderer> renderer(new GameRenderer(config));
	
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
		// Reload resources.
		if(Input::manager().triggered(Input::KeyP)){
			Resources::manager().reload();
		}
		
		// Start a new frame for the interface.
		Interface::beginFrame();
		
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
		Interface::endFrame();
		//Display the result for the current rendering loop.
		glfwSwapBuffers(window);
		
	}
	
	// Clean the interface.
	Interface::clean();
	// Remove the window.
	glfwDestroyWindow(window);
	// Clean other resources
	renderer->clean();
	// Close GL context and any other GLFW resources.
	glfwTerminate();
	
	return 0;
}


