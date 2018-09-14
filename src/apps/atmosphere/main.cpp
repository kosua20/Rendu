#include "Common.hpp"
#include "helpers/GenerationUtilities.hpp"
#include "input/Input.hpp"
#include "input/InputCallbacks.hpp"
#include "input/ControllableCamera.hpp"
#include "helpers/InterfaceUtilities.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/ScreenQuad.hpp"
#include "Config.hpp"

/**
 \defgroup Atmosphere Atmospheric scattering demo
 \brief Demonstrate real-time approximate atmospheric scattering simulation.
 \see GLSL::Frag::Atmosphere
 \ingroup Applications
 */


/**
 The main function of the atmospheric scattering demo.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup Atmosphere
 */
int main(int argc, char** argv) {
	
	// First, init/parse/load configuration.
	Config config(argc, argv);
	if(!config.logPath.empty()){
		Log::setDefaultFile(config.logPath);
	}
	Log::setDefaultVerbose(config.logVerbose);
	
	GLFWwindow* window = Interface::initWindow("Atmosphere", config);
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
	
	glEnable(GL_DEPTH_TEST);
	
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
	std::shared_ptr<Framebuffer> atmosphereFramebuffer(new Framebuffer(renderResolution[0], renderResolution[1], GL_RGB32F, true));
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
		Interface::beginFrame();
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
		Interface::endFrame();
		//Display the result for the current rendering loop.
		glfwSwapBuffers(window);
		
	}
	
	// Cleaning.
	atmosphereFramebuffer->clean();
	
	// Clean the interface.
	Interface::clean();
	// Remove the window.
	glfwDestroyWindow(window);
	// Close GL context and any other GLFW resources.
	glfwTerminate();
	
	return 0;
}


