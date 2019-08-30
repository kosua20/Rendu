#include "input/Input.hpp"
#include "input/ControllableCamera.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/GLUtilities.hpp"
#include "system/Config.hpp"
#include "system/System.hpp"
#include "system/Random.hpp"
#include "Common.hpp"

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
int main(int argc, char ** argv) {

	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}

	GLFWwindow * window = System::initWindow("Playground", config);
	if(!window) {
		return -1;
	}

	Resources::manager().addResources("../../../resources/common");

	// Seed random generator.
	Random::seed();
	// Query the renderer identifier, and the supported OpenGL version.
	std::string vendor, renderer, version, shaderVersion;
	GLUtilities::deviceInfos(vendor, renderer, version, shaderVersion);
	Log::Info() << Log::OpenGL << "Vendor: " << vendor << "." << std::endl;
	Log::Info() << Log::OpenGL << "Internal renderer: " << renderer << "." << std::endl;
	Log::Info() << Log::OpenGL << "Versions: Driver: " << version << ", GLSL: " << shaderVersion << "." << std::endl;

	// Query the extensions.
	const std::vector<std::string> extensions = GLUtilities::deviceExtensions();
	// Log extensions.
	if(!extensions.empty()) {
		Log::Info() << Log::OpenGL << "Extensions detected (" << extensions.size() << ")" << std::flush;
		for(size_t i = 0; i < extensions.size(); ++i) {
			Log::Verbose() << (i == 0 ? ": " : ", ") << extensions[i] << std::flush;
		}
		Log::Info() << std::endl;
	}
	const std::string titleHeader = "Extensions (" + std::to_string(extensions.size()) + ")";

	glEnable(GL_DEPTH_TEST);

	// Setup the timer.
	double timer		 = glfwGetTime();
	double fullTime		 = 0.0;
	double remainingTime = 0.0;
	const double dt		 = 1.0 / 120.0; // Small physics timestep.

	const Program * program = Resources::manager().getProgram("object", "object_basic", "object_basic_random");
	const Mesh * mesh		= Resources::manager().getMesh("light_sphere", Storage::GPU);
	ControllableCamera camera;
	camera.projection(config.screenResolution[0] / config.screenResolution[1], 1.34f, 0.1f, 100.0f);
	bool showImGuiDemo = false;

	// Start the display/interaction loop.
	while(!glfwWindowShouldClose(window)) {
		// Update events (inputs,...).
		Input::manager().update();
		// Handle quitting.
		if(Input::manager().pressed(Input::KeyEscape)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

		// Reload resources.
		if(Input::manager().triggered(Input::KeyP)) {
			Resources::manager().reload();
		}

		// Compute the time elapsed since last frame
		const double currentTime = glfwGetTime();
		double frameTime		 = currentTime - timer;
		timer					 = currentTime;

		camera.update();

		// Physics simulation
		// First avoid super high frametime by clamping.
		if(frameTime > 0.2) {
			frameTime = 0.2;
		}
		// Accumulate new frame time.
		remainingTime += frameTime;
		// Instead of bounding at dt, we lower our requirement (1 order of magnitude).
		while(remainingTime > 0.2 * dt) {
			double deltaTime = fmin(remainingTime, dt);
			// Update physics and camera.
			camera.physics(deltaTime);
			// Update timers.
			fullTime += deltaTime;
			remainingTime -= deltaTime;
		}

		// Start a new frame for the interface.
		System::Gui::beginFrame();
		// Render.
		const glm::ivec2 screenSize = Input::manager().size();
		const glm::mat4 MVP		   = camera.projection() * camera.view();
		GLUtilities::setViewport(0, 0, screenSize[0], screenSize[1]);
		GLUtilities::clearColorAndDepth({0.2f, 0.3f, 0.25f, 1.0f}, 1.0f);
		program->use();
		program->uniform("mvp", MVP);
		GLUtilities::drawMesh(*mesh);

		ImGui::Text("ImGui is functional!");
		ImGui::SameLine();
		ImGui::Checkbox("Show demo", &showImGuiDemo);
		ImGui::Text("%.1f FPS / %.1f ms", ImGui::GetIO().Framerate, ImGui::GetIO().DeltaTime * 1000.0f);
		ImGui::Separator();

		ImGui::Text("OpengGL vendor: %s", vendor.c_str());
		ImGui::Text("Internal renderer: %s", renderer.c_str());
		ImGui::Text("Versions: Driver: %s, GLSL: %s", version.c_str(), shaderVersion.c_str());
		if(ImGui::CollapsingHeader(titleHeader.c_str())) {
			for(const auto & ext : extensions) {
				ImGui::Text("%s", ext.c_str());
			}
		}

		if(showImGuiDemo) {
			ImGui::ShowDemoWindow();
		}

		// Then render the interface.
		System::Gui::endFrame();
		//Display the result for the current rendering loop.
		glfwSwapBuffers(window);
	}

	// Clean the interface.
	System::Gui::clean();

	Resources::manager().clean();
	// Close GL context and any other GLFW resources.
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
