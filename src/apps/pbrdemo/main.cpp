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

	// Start the display/interaction loop.
	while(window.nextFrame()) {
		
		// Reload resources.
		if(Input::manager().triggered(Input::Key::P)) {
			Resources::manager().reload();
		}
		app.update();
		app.draw();
	}

	// Clean other resources
	app.clean();
	Resources::manager().clean();
	
	window.clean();

	return 0;
}
