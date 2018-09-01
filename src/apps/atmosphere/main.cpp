#include "Common.hpp"
#include "helpers/GenerationUtilities.hpp"
#include "input/Input.hpp"
#include "input/ControllableCamera.hpp"
#include "helpers/InterfaceUtilities.hpp"
#include "resources/ResourcesManager.hpp"
#include "ScreenQuad.hpp"
#include "Config.hpp"
#include "resources/ImageUtilities.hpp"

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
		window = glfwCreateWindow(mode->width, mode->height, "Atmosphere", glfwGetPrimaryMonitor(), NULL);
	} else {
		// Create a window with a given size. Width and height are defined in the configuration.
		window = glfwCreateWindow(config.initialWidth, config.initialHeight,"Atmosphere", NULL, NULL);
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
	const GLubyte* rendererString = glGetString(GL_RENDERER);
	const GLubyte* versionString = glGetString(GL_VERSION);
	Log::Info() << Log::OpenGL << "Internal renderer: " << rendererString << "." << std::endl;
	Log::Info() << Log::OpenGL << "Version supported: " << versionString << "." << std::endl;
	
	
	
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
	
	// Camera.
	ControllableCamera camera;
	camera.projection(config.screenResolution[0]/config.screenResolution[1], 1.34f, 0.1f, 100.0f);
	const glm::vec2 renderResolution = (config.internalVerticalResolution/config.screenResolution[1]) * config.screenResolution;
	
	// Framebuffer to store the rendered atmosphere result before tonemapping and upscaling to the window size.
	std::shared_ptr<Framebuffer> atmosphereFramebuffer(new Framebuffer(renderResolution[0], renderResolution[1], GL_RGB, GL_FLOAT, GL_RGB32F, GL_LINEAR, GL_CLAMP_TO_EDGE, true));
	const GLuint precomputedScattering = Resources::manager().getTexture("scattering-precomputed", false).id;
	
	// Atmosphere screen quad.
	std::shared_ptr<ProgramInfos> atmosphereProgram = Resources::manager().getProgram2D("atmosphere");
	
	// Final tonemapping screen quad.
	std::shared_ptr<ProgramInfos> tonemapProgram = Resources::manager().getProgram2D("tonemap");
	
	// Sun direction.
	glm::vec3 lightDirection(0.437f,0.082f,-0.896f);
	lightDirection = glm::normalize(lightDirection);
	
	// Timing.
	const unsigned int frameCount = 20;
	std::vector<double> timings(frameCount, 0.0);
	unsigned int currentTiming = 0;
	double smoothedFrameTime = 0.0;
	
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
		
		// Timing.
		smoothedFrameTime -= timings[currentTiming];
		timings[currentTiming] = 1000.0*frameTime;
		smoothedFrameTime += timings[currentTiming];
		currentTiming = (currentTiming+1)%frameCount;
		const double sft = smoothedFrameTime/frameCount;
		ImGui::Text("%2.2f ms (%2.0f fps)", sft, 1.0/sft*1000.0);
		
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
		// Handle resizing directly.
		const glm::vec2 screenSize = Input::manager().size();
		if(Input::manager().resized()){
			atmosphereFramebuffer->resize(screenSize);
		}
		
		// Render.
		const glm::mat4 camToWorld = glm::inverse(camera.view());
		const glm::mat4 clipToCam = glm::inverse(camera.projection());
		
		// Draw the atmosphere.
		glDisable(GL_DEPTH_TEST);
		atmosphereFramebuffer->bind();
		atmosphereFramebuffer->setViewport();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glUseProgram(atmosphereProgram->id());
		const glm::mat4 camToWorldNoT = glm::mat4(glm::mat3(camToWorld));
		const glm::mat4 clipToWorld = camToWorldNoT * clipToCam;
		glUniformMatrix4fv(atmosphereProgram->uniform("clipToWorld"), 1, GL_FALSE, &clipToWorld[0][0]);
		glUniform3fv(atmosphereProgram->uniform("viewPos"), 1, &camera.position()[0]);
		glUniform3fv(atmosphereProgram->uniform("lightDirection"), 1, &lightDirection[0]);
		ScreenQuad::draw(precomputedScattering);
		atmosphereFramebuffer->unbind();
		
		// Tonemapping and final screen.
		glViewport(0, 0, (GLsizei)screenSize[0], (GLsizei)screenSize[1]);
		glEnable(GL_FRAMEBUFFER_SRGB);
		glUseProgram(tonemapProgram->id());
		ScreenQuad::draw(atmosphereFramebuffer->textureId());
		glDisable(GL_FRAMEBUFFER_SRGB);
		
		// Settings.
		if(ImGui::DragFloat3("Light dir", &lightDirection[0], 0.05f, -1.0f, 1.0f)){
			lightDirection = glm::normalize(lightDirection);
		}
		// Then render the interface.
		ImGui::endFrame();
		//Display the result for the current rendering loop.
		glfwSwapBuffers(window);
		
	}
	
	// Cleaning.
	atmosphereFramebuffer->clean();
	
	// Clean the interface.
	ImGui::clean();
	// Remove the window.
	glfwDestroyWindow(window);
	// Close GL context and any other GLFW resources.
	glfwTerminate();
	
	return 0;
}


