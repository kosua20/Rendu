
#include "Common.hpp"
#include "helpers/Random.hpp"
#include "input/Input.hpp"
#include "helpers/System.hpp"
#include "resources/ResourcesManager.hpp"
#include "Game.hpp"


/**
 \defgroup SnakeGame Snake Game
 \brief A small 3D game demo.
 \ingroup Applications
 */

/**
 The main function of the game demo. Handles the setup and main loop.
 \return a general error code.
 \ingroup SnakeGame
 */
int main() {
	
	// First, init/parse/load configuration.
	const std::vector<std::string> forceArgv = {"SnakeGame", "-c", "config.ini"};
	RenderingConfig config(forceArgv);
	config.initialWidth = 800;
	config.initialHeight = 600;
	config.forceAspectRatio = true;
	
	GLFWwindow* window = System::initWindow("SnakeGame", config);
	if(!window){
		return -1;
	}
	// Disable Imgui saving.
	ImGui::GetIO().IniFilename = NULL;

	Resources::manager().addResources("../../../resources/common");
	Resources::manager().addResources("../../../resources/snakegame");
	// Initialize random generator;
	Random::seed();
	
	// Create the game main handler..
	Game game(config);
	// Make sure the score file exists.
	if(!Resources::externalFileExists("./scores.sav")){
		Resources::saveStringToExternalFile("./scores.sav", "\n");
	}
	
	double timer = glfwGetTime();
	double remainingTime = 0.0;
	const double dt = 1.0/120.0; // Small physics timestep.
	
	// Start the display/interaction loop.
	while (!glfwWindowShouldClose(window)) {
		// Update events (inputs,...).
		Input::manager().update();
		
		
		// Start a new frame for the interface.
		System::GUI::beginFrame();
		
		// We separate punctual events from the main physics/movement update loop.
		const System::Action actionToTake = game.update();
		if(actionToTake != System::Action::None){
			System::performWindowAction(window, config, actionToTake);
			// Due to the ordering between the update function and the fullscreen activation, we have to manually call resize here.
			// Another solution would be to check resizing before rendering, in the Game object.
			if(actionToTake == System::Action::Fullscreen){
				game.resize((unsigned int)Input::manager().size()[0], (unsigned int)Input::manager().size()[1]);
			}
			// Update the config on disk, for next launch.
			Resources::saveStringToExternalFile("./config.ini", "# SnakeGame Config v1.0\n" + std::string(config.fullscreen ? "fullscreen\n" : "") + (!config.vsync ? "no-vsync" : "\n"));
		}
		
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
			game.physics(deltaTime);
			// Update timers.
			remainingTime -= deltaTime;
		}
		
		// Update the content of the window.
		game.draw();
		// Then render the interface.
		System::GUI::endFrame();
		//Display the result for the current rendering loop.
		glfwSwapBuffers(window);
		
	}
	
	// Clean the interface.
	System::GUI::clean();
	// Clean other resources
	game.clean();
	Resources::manager().clean();
	// Close GL context and any other GLFW resources.
	glfwDestroyWindow(window);
	glfwTerminate();
	
	return 0;
}


