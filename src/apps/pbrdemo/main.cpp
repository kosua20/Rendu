#include "PBRDemo.hpp"
#include "system/Random.hpp"
#include "input/Input.hpp"
#include "system/Window.hpp"
#include "system/System.hpp"
#include "scene/Scene.hpp"
#include "Common.hpp"

/**
 \defgroup DeferredRendering Deferred rendering
 \brief Render a scene using a PBR approach, image-based ambient lighting, ambient occlusion, HDR and tonemapping, antialiasing, and deferred rendering.
 \details All scene informations (albedo, normals, material ID, roughness) are rendered to a G-Buffer before being used to render each light contribution using simple geometric proxies. The scene is rendered using the GGX BRDF model.
 \see GPU::Frag::Bloom, GPU::Frag::Tonemap, GPU::Frag::Fxaa, GPU::Frag::Final_screenquad, GPU::Frag::Ambient, GPU::Frag::Ssao, GPU::Frag::Point_light, GPU::Frag::Directional_light, GPU::Frag::Spot_light
 \ingroup Applications
 */

/**
 The main function of the physically-based rendering demo. Handles the setup and main loop.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup DeferredRendering
 */
int main(int argc, char ** argv) {

	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}

	Window window("PBR demo", config);
	
	Resources::manager().addResources("../../../resources/common");
	Resources::manager().addResources("../../../resources/pbrdemo");
	Resources::manager().addResources("../../../resources/additional");

	// Seed random generator.
	Random::seed();

	// Create the renderer.
	PBRDemo app(config);

	double timer		 = System::time();
	double fullTime		 = 0.0;
	double remainingTime = 0.0;
	const double dt		 = 1.0 / 120.0; // Small physics timestep.

	// Start the display/interaction loop.
	while(window.nextFrame()) {
		
		// Reload resources.
		if(Input::manager().triggered(Input::Key::P)) {
			Resources::manager().reload();
		}

		// We separate punctual events from the main physics/movement update loop.
		app.update();

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
			app.physics(fullTime, deltaTime);
			// Update timers.
			fullTime += deltaTime;
			remainingTime -= deltaTime;
		}

		// Update the content of the window.
		app.draw();
	}

	// Clean other resources
	app.clean();
	Resources::manager().clean();
	
	window.clean();

	return 0;
}
