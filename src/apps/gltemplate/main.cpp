#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h> // to set up the OpenGL context and manage window lifecycle and inputs


#include "helpers/GenerationUtilities.hpp"
#include "input/Input.hpp"
#include "renderers/deferred/DeferredRenderer.hpp"
#include "renderers/utils/RendererCube.hpp"
#include "renderers/utils/TestRenderer.hpp"
#include "helpers/Logger.hpp"
#include "helpers/InterfaceUtilities.hpp"
#include "scenes/Scenes.hpp"

#include <stdio.h>
#include <memory>

/// Callbacks

void resize_callback(GLFWwindow* window, int width, int height){
	Input::manager().resizeEvent(width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	if(!ImGui::GetIO().WantCaptureKeyboard){
		Input::manager().keyPressedEvent(key, action);
	}
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
	if(!ImGui::GetIO().WantCaptureMouse){
		Input::manager().mousePressedEvent(button, action);
	}
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos){
	if(!ImGui::GetIO().WantCaptureMouse){
		Input::manager().mouseMovedEvent(xpos, ypos);
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
	if(!ImGui::GetIO().WantCaptureMouse){
		Input::manager().mouseScrolledEvent(xoffset, yoffset);
	}
	ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

void joystick_callback(int joy, int event){
	Input::manager().joystickEvent(joy, event);
}



/// The main function

int main(int argc, char** argv) {
	
	// First, init/parse/load configuration.
	Config config(argc, argv);
	if(!config.logPath.empty()){
		Log::setDefaultFile(config.logPath);
	}
	Log::setDefaultVerbose(config.logVerbose);
	
	// Initialize glfw, which will create and setup an OpenGL context.
	if (!glfwInit()) {
		Log::Error() << Log::OpenGL << "Could not start GLFW3" << std::endl;
		return 1;
	}

	glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	GLFWwindow* window;
	
	if(config.fullscreen){
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		window = glfwCreateWindow(mode->width, mode->height, "GL_Template", glfwGetPrimaryMonitor(), NULL);
	} else {
		// Create a window with a given size. Width and height are defined in the configuration.
		window = glfwCreateWindow(config.initialWidth, config.initialHeight,"GL_Template", NULL, NULL);
	}
	
	if (!window) {
		Log::Error() << Log::OpenGL << "Could not open window with GLFW3" << std::endl;
		glfwTerminate();
		return 1;
	}

	// Bind the OpenGL context and the new window.
	glfwMakeContextCurrent(window);

	if (gl3wInit()) {
		Log::Error() << Log::OpenGL << "Failed to initialize OpenGL" << std::endl;
		return -1;
	}
	if (!gl3wIsSupported(3, 2)) {
		Log::Error() << Log::OpenGL << "OpenGL 3.2 not supported\n" << std::endl;
		return -1;
	}

	// Setup callbacks for various interactions and inputs.
	glfwSetFramebufferSizeCallback(window, resize_callback);	// Resizing the window
	glfwSetKeyCallback(window,key_callback);					// Pressing a key
	glfwSetMouseButtonCallback(window,mouse_button_callback);	// Clicking the mouse buttons
	glfwSetCursorPosCallback(window,cursor_pos_callback);		// Moving the cursor
	glfwSetScrollCallback(window,scroll_callback);				// Scrolling
	glfwSetJoystickCallback(joystick_callback);					// Joystick
	glfwSwapInterval(config.vsync ? 1 : 0);						// 60 FPS V-sync
	
	ImGui::setup(window);
	
	// Check the window size (if we are on a screen smaller than the initial size.
	int wwidth, wheight;
	glfwGetWindowSize(window, &wwidth, &wheight);
	config.initialWidth = wwidth;
	config.initialHeight = wheight;
	
	// On HiDPI screens, we have to consider the internal resolution for all framebuffers size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	config.screenResolution = glm::vec2(width, height);
	// Compute point density by computing the ratio.
	config.screenDensity = (float)width/(float)config.initialWidth;
	// Update the resolution.
	Input::manager().resizeEvent(width, height);
	
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
		// Start a new frame for the interface.
		ImGui::beginFrame();
		// Reload resources.
		if(Input::manager().triggered(Input::KeyP)){
			Resources::manager().reload();
		}
		
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
		ImGui::endFrame();
		//Display the result for the current rendering loop.
		glfwSwapBuffers(window);

	}
	
	// Clean the interface.
	ImGui::clean();
	// Remove the window.
	glfwDestroyWindow(window);
	// Clean other resources
	renderer->clean();
	// Close GL context and any other GLFW resources.
	glfwTerminate();
	
	return 0;
}


