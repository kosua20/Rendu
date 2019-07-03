#include "Interface.hpp"
#include "input/InputCallbacks.hpp"
#include "input/Input.hpp"

#include <nfd/nfd.h>
#include <imgui/imgui_impl_opengl3.h>


namespace Interface {
	

	void setupImGui(GLFWwindow * window){
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui_ImplGlfw_InitForOpenGL(window, false);
		ImGui_ImplOpenGL3_Init("#version 150");
		ImGui::StyleColorsDark();
	}
		
	void beginFrame(){
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}
	
	void endFrame(){
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
	
	void clean(){
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
	
	GLFWwindow* initWindow(const std::string & name, RenderingConfig & config){
		// Initialize glfw, which will create and setup an OpenGL context.
		if (!glfwInit()) {
			Log::Error() << Log::OpenGL << "Could not start GLFW3" << std::endl;
			return nullptr;
		}
		
		glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
		glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		
		GLFWwindow * window;
		
		if(config.fullscreen){
			const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
			/// \note We might want to impose the configured size here. This means the monitor could be set in a non-native mode.
			window = glfwCreateWindow(mode->width, mode->height, name.c_str(), glfwGetPrimaryMonitor(), NULL);
		} else {
			// Create a window with a given size. Width and height are defined in the configuration.
			window = glfwCreateWindow(config.initialWidth, config.initialHeight, name.c_str(), NULL, NULL);
		}
		
		if (!window) {
			Log::Error() << Log::OpenGL << "Could not open window with GLFW3" << std::endl;
			glfwTerminate();
			return nullptr;
		}
		
		if(config.forceAspectRatio){
			glfwSetWindowAspectRatio(window, config.initialWidth, config.initialHeight);
		}
		// Bind the OpenGL context and the new window.
		glfwMakeContextCurrent(window);
		
		if (gl3wInit()) {
			Log::Error() << Log::OpenGL << "Failed to initialize OpenGL" << std::endl;
			return nullptr;
		}
		if (!gl3wIsSupported(3, 2)) {
			Log::Error() << Log::OpenGL << "OpenGL 3.2 not supported\n" << std::endl;
			return nullptr;
		}
		
		// Setup callbacks for various interactions and inputs.
		glfwSetFramebufferSizeCallback(window, resize_callback);	// Resizing the window
		glfwSetKeyCallback(window, key_callback);					// Pressing a key
		glfwSetCharCallback(window, char_callback);					// Outputing a text char (for ImGui)
		glfwSetMouseButtonCallback(window, mouse_button_callback);	// Clicking the mouse buttons
		glfwSetCursorPosCallback(window, cursor_pos_callback);		// Moving the cursor
		glfwSetScrollCallback(window, scroll_callback);				// Scrolling
		glfwSetJoystickCallback(joystick_callback);					// Joystick
		glfwSetWindowIconifyCallback(window, iconify_callback); 	// Window minimization
		glfwSwapInterval(config.vsync ? (config.rate == 30 ? 2 : 1) : 0);						// 60 FPS V-sync
		
		setupImGui(window);
		
		// Check the window position and size (if we are on a screen smaller than the initial size).
		glfwGetWindowPos(window, &config.windowFrame[0], &config.windowFrame[1]);
		glfwGetWindowSize(window, &config.windowFrame[2], &config.windowFrame[3]);
		
		// On HiDPI screens, we have to consider the internal resolution for all framebuffers size.
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		config.screenResolution = glm::vec2(width, height);
		
		// Compute point density by computing the ratio.
		const float screenDensity = (float)width/(float)config.windowFrame[2];
		Input::manager().densityEvent(screenDensity);
		
		// Update the resolution.
		Input::manager().resizeEvent(width, height);
		
		// Default OpenGL state, just in case.
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_BLEND);
		
		return window;
	}
	
	void performWindowAction(GLFWwindow * window, RenderingConfig & config, const Action action){
		switch (action) {
		case Action::Quit :
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		case Action::Vsync:
			{
				if(config.vsync){
					glfwSwapInterval(0);
				} else {
					glfwSwapInterval(config.rate == 30 ? 2 : 1);
				}
				config.vsync = !config.vsync;
				break;
			}
		case Action::Fullscreen:
			{
				// Are we currently fullscreen?
				const bool fullscreen = glfwGetWindowMonitor(window) != nullptr;
				
				if(fullscreen){
					// Restore the window position and size.
					glfwSetWindowMonitor(window, NULL, config.windowFrame[0], config.windowFrame[1], config.windowFrame[2], config.windowFrame[3], 0);
					// Check the window position and size (if we are on a screen smaller than the initial size).
					glfwGetWindowPos(window, &config.windowFrame[0], &config.windowFrame[1]);
					glfwGetWindowSize(window, &config.windowFrame[2], &config.windowFrame[3]);
				} else {
					// Backup the window current frame.
					glfwGetWindowPos(window, &config.windowFrame[0], &config.windowFrame[1]);
					glfwGetWindowSize(window, &config.windowFrame[2], &config.windowFrame[3]);
					// Move to fullscreen on the primary monitor.
					GLFWmonitor* monitor = glfwGetPrimaryMonitor();
					const GLFWvidmode* mode = glfwGetVideoMode(monitor);
					glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
				}
				config.fullscreen = !fullscreen;
				// On some hardware, V-sync options can be lost.
				glfwSwapInterval(config.vsync ? (config.rate == 30 ? 2 : 1) : 0);

				// On HiDPI screens, we have to consider the internal resolution for all framebuffers size.
				int width, height;
				glfwGetFramebufferSize(window, &width, &height);
				config.screenResolution = glm::vec2(width, height);
				
				int wwidth, hheight;
				glfwGetWindowSize(window, &wwidth, &hheight);
				// Compute point density by computing the ratio.
				const float screenDensity = (float)width/(float)wwidth;
				Input::manager().densityEvent(screenDensity);
				
				// Update the resolution.
				Input::manager().resizeEvent(width, height);
			}
			break;
		default:
			break;
		}
	}
	
	bool showPicker(const Picker mode, const std::string & startPath, std::string & outPath, const std::string & extensions){
		nfdchar_t *outPathRaw = NULL;
		nfdresult_t result = NFD_CANCEL;
		outPath = "";
		
#ifdef _WIN32
		const std::string internalStartPath = "";
#else
		const std::string internalStartPath = startPath;
#endif
		if(mode == Picker::Load){
			result = NFD_OpenDialog(extensions.empty() ? NULL : extensions.c_str(), internalStartPath.c_str(), &outPathRaw);
		} else if(mode == Picker::Save){
			result = NFD_SaveDialog(extensions.empty() ? NULL : extensions.c_str(), internalStartPath.c_str(), &outPathRaw);
		} else if(mode == Picker::Directory){
			result = NFD_PickFolder(internalStartPath.c_str(), &outPathRaw);
		}
		
		if (result == NFD_OKAY) {
			outPath = std::string(outPathRaw);
			free(outPathRaw);
			return true;
		} else if (result == NFD_CANCEL) {
			// Cancelled by user, nothing to do.
		} else {
			// Real error.
			Log::Error() << "Unable to present system picker (" <<  std::string(NFD_GetError()) << ")." << std::endl;
		}
		free(outPathRaw);
		return false;
	}
}







