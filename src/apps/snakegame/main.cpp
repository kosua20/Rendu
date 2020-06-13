#include "Game.hpp"

#include "input/Input.hpp"
#include "generation/Random.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "resources/ResourcesManager.hpp"
#include "Common.hpp"

/**
 \defgroup SnakeGame Snake Game
 \brief A 3D snake game demo.
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
	GameConfig config(forceArgv);
	config.initialWidth		= 800;
	config.initialHeight	= 600;
	config.forceAspectRatio = true;

	Window window("SnakeGame", config, false);
	
	// Disable Imgui saving.
	ImGui::GetIO().IniFilename = nullptr;

	Resources::manager().addResources("../../../resources/common");
	Resources::manager().addResources("../../../resources/snakegame");
	// Seed random generator.
	Random::seed();

	// Create the game main handler..
	Game game(config);
	// Make sure the score file exists.
	if(!Resources::externalFileExists("./scores.sav")) {
		Resources::saveStringToExternalFile("./scores.sav", "\n");
	}

	double timer		 = System::time();
	double remainingTime = 0.0;
	const double dt		 = 1.0 / 120.0; // Small physics timestep.

	// Start the display/interaction loop.
	while(window.nextFrame()) {

		// We separate punctual events from the main physics/movement update loop.
		const Window::Action actionToTake = game.update();
		if(actionToTake != Window::Action::None) {
			window.perform(actionToTake);
			// Due to the ordering between the update function and the fullscreen activation, we have to manually call resize here.
			// Another solution would be to check resizing before rendering, in the Game object.
			if(actionToTake == Window::Action::Fullscreen) {
				game.resize(uint(Input::manager().size()[0]), uint(Input::manager().size()[1]));
			}
			// Update the config on disk, for next launch.
			config.save();
		}

		// Compute the time elapsed since last frame
		const double currentTime = System::time();
		double frameTime		 = currentTime - timer;
		timer					 = currentTime;

		// Physics simulation
		// First avoid super high frametime by clamping.
		if(frameTime > 0.2) {
			frameTime = 0.2;
		}
		// Accumulate new frame time.
		remainingTime += frameTime;
		// Instead of bounding at dt, we lower our requirement (1 order of magnitude).
		while(remainingTime > 0.2 * dt) {
			const double deltaTime = std::min(remainingTime, dt);
			// Update physics and camera.
			game.physics(deltaTime);
			// Update timers.
			remainingTime -= deltaTime;
		}

		// Update the content of the window.
		game.draw();
		
	}

	// Clean resources.
	Resources::manager().clean();

	return 0;
}
