#include "Common.hpp"
#include "helpers/GenerationUtilities.hpp"
#include "input/Input.hpp"
#include "input/InputCallbacks.hpp"
#include "renderers/deferred/DeferredRenderer.hpp"
#include "renderers/utils/RendererCube.hpp"
#include "helpers/InterfaceUtilities.hpp"
#include "scenes/Scenes.hpp"

/**
 \defgroup PBRDemo Physically-based rendering demo
 \brief Render a scene using the GGX BRDF model, image-based ambient lighting, ambient occlusion, HDR and tonemapping, antialiasing.
 \see GLSL::Frag::Ambient
 \see GLSL::Frag::Ssao
 \see GLSL::Frag::Tonemap
 \see GLSL::Frag::Fxaa
 \see GLSL::Frag::Point_light
 \see GLSL::Frag::Directional_light
 \see GLSL::Frag::Spot_light
 \ingroup Applications
 */

/**
 The main function of the physically-based rendering demo. Handles the setup and main loop.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup PBRDemo
 */
int main(int argc, char** argv) {
	
	// First, init/parse/load configuration.
	RenderingConfig config(argc, argv);
	
	GLFWwindow* window = Interface::initWindow("PBR demo", config);
	if(!window){
		return -1;
	}
	
	// Initialize random generator;
	Random::seed();
	// Query the renderer identifier, and the supported OpenGL version.
	const GLubyte* rendererString = glGetString(GL_RENDERER);
	const GLubyte* versionString = glGetString(GL_VERSION);
	Log::Info() << Log::OpenGL << "Internal renderer: " << rendererString << "." << std::endl;
	Log::Info() << Log::OpenGL << "Version supported: " << versionString << "." << std::endl;
	
	
	
	// Create the renderer.
	std::shared_ptr<DeferredRenderer> renderer(new DeferredRenderer(config));
	
	double timer = glfwGetTime();
	double fullTime = 0.0;
	double remainingTime = 0.0;
	const double dt = 1.0/120.0; // Small physics timestep.
	
	std::vector<std::shared_ptr<Scene>> scenes;
	scenes.emplace_back(new DragonScene());
	scenes.emplace_back(new SphereScene());
	scenes.emplace_back(new DeskScene());
	char const * sceneNames[] = {"Dragon", "Spheres", "Desk", "None"};
	// Load the first scene by default.
	int selected_scene = 0;
	renderer->setScene(scenes[selected_scene]);
	
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
		
		// Start a new frame for the interface.
		Interface::beginFrame();
		
		// Handle scene switching.
		if(ImGui::Begin("Renderer")){
			if(ImGui::Combo("Scene", &selected_scene, sceneNames, scenes.size()+1)){
				if(selected_scene == scenes.size()){
					renderer->setScene(nullptr);
				} else {
					Log::Info() << Log::Resources << "Loading scene " << sceneNames[selected_scene] << "." << std::endl;
					renderer->setScene(scenes[selected_scene]);
				}
			}
		}
		ImGui::End();
		
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


