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
	RenderingConfig config(argc, argv);
	
	GLFWwindow* window = Interface::initWindow("Playground", config);
	if(!window){
		return -1;
	}
	
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
			Log::Verbose() << (i == 0 ? ": " : ", ") << rendererString << std::flush;
		}
		Log::Info() << std::endl;
	}
	
	glEnable(GL_DEPTH_TEST);
	
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
		Interface::endFrame();
		//Display the result for the current rendering loop.
		glfwSwapBuffers(window);

	}
	
	// Clean the interface.
	Interface::clean();
	// Remove the window.
	glfwDestroyWindow(window);
	// Close GL context and any other GLFW resources.
	glfwTerminate();
	
	return 0;
}


