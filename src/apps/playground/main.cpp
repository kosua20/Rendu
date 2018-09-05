#include "Common.hpp"
#include "helpers/GenerationUtilities.hpp"
#include "input/Input.hpp"
#include "input/InputCallbacks.hpp"
#include "input/ControllableCamera.hpp"
#include "helpers/InterfaceUtilities.hpp"
#include "resources/ResourcesManager.hpp"
#include "ScreenQuad.hpp"
#include "Config.hpp"

/**
 \defgroup Playground Playground
 \brief A basic playground for testing ideas.
 \ingroup Applications
 */

/**
 The main function of the playground.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup Playground
 */
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
		window = glfwCreateWindow(mode->width, mode->height, "Playground", glfwGetPrimaryMonitor(), NULL);
	} else {
		// Create a window with a given size. Width and height are defined in the configuration.
		window = glfwCreateWindow(config.initialWidth, config.initialHeight,"Playground", NULL, NULL);
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
	
	// Check the window size (if we are on a screen smaller than the initial size).
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
	const GLubyte* vendorString = glGetString(GL_VENDOR);
	const GLubyte* rendererString = glGetString(GL_RENDERER);
	const GLubyte* versionString = glGetString(GL_VERSION);
	const GLubyte* glslVersionString = glGetString(GL_SHADING_LANGUAGE_VERSION);
	Log::Info() << Log::OpenGL << "Vendor: " << vendorString << "." << std::endl;
	Log::Info() << Log::OpenGL << "Internal renderer: " << rendererString << "." << std::endl;
	Log::Info() << Log::OpenGL << "Versions: Driver: " << versionString << ", GLSL: " << glslVersionString << "." << std::endl;
	
	// Query the extensions.
	int extensionCount = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
	if(extensionCount>0){
		Log::Info() << Log::OpenGL << "Extensions detected (" << extensionCount << ")" << std::flush;
		for(int i = 0; i < extensionCount; ++i){
			const GLubyte* rendererString = glGetStringi(GL_EXTENSIONS, i);
			Log::Info() << (i == 0 ? ": " : ", ") << rendererString << std::flush;
		}
		Log::Info() << std::endl;
	}
	
	// Default OpenGL state.
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glDisable(GL_BLEND);
	
	// Setup the timer.
	double timer = glfwGetTime();
	double fullTime = 0.0;
	double remainingTime = 0.0;
	const double dt = 1.0/120.0; // Small physics timestep.
	
	std::shared_ptr<ProgramInfos> program = Resources::manager().getProgram("object_basic");
	MeshInfos mesh = Resources::manager().getMesh("light_sphere");
	ControllableCamera camera;
	camera.projection(config.screenResolution[0]/config.screenResolution[1], 1.34f, 0.1f, 100.0f);
	
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
		
		// Compute the time elapsed since last frame
		const double currentTime = glfwGetTime();
		double frameTime = currentTime - timer;
		timer = currentTime;
		camera.update();
		
		// Physics simulation
		// First avoid super high frametime by clamping.
		if(frameTime > 0.2){ frameTime = 0.2; }
		// Accumulate new frame time.
		remainingTime += frameTime;
		// Instead of bounding at dt, we lower our requirement (1 order of magnitude).
		while(remainingTime > 0.2*dt){
			double deltaTime = fmin(remainingTime, dt);
			// Update physics and camera.
			camera.physics(deltaTime);
			// Update timers.
			fullTime += deltaTime;
			remainingTime -= deltaTime;
		}
		
		// Render.
		const glm::vec2 screenSize = Input::manager().size();
		const glm::mat4 MVP = camera.projection() * camera.view();
		glViewport(0, 0, (GLsizei)screenSize[0], (GLsizei)screenSize[1]);
		glClearColor(0.2f, 0.3f, 0.25f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(program->id());
		glUniformMatrix4fv(program->uniform("mvp"), 1, GL_FALSE, &MVP[0][0]);
		glBindVertexArray(mesh.vId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.eId);
		glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, (void*)0);
		glBindVertexArray(0);
		glUseProgram(0);
		ImGui::Text("ImGui is functional!");
		
		// Then render the interface.
		ImGui::endFrame();
		//Display the result for the current rendering loop.
		glfwSwapBuffers(window);

	}
	
	// Clean the interface.
	ImGui::clean();
	// Remove the window.
	glfwDestroyWindow(window);
	// Close GL context and any other GLFW resources.
	glfwTerminate();
	
	return 0;
}


