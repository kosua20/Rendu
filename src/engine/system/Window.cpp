#include "Window.hpp"

#include "input/InputCallbacks.hpp"
#include "input/Input.hpp"
#include "graphics/GLUtilities.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_glfw.h>

#include <GLFW/glfw3.h>

Window::Window(const std::string & name, RenderingConfig & config, bool escapeQuit, bool hidden) :
	_config(config), _allowEscape(escapeQuit) {
	// Initialize glfw, which will create and setup an OpenGL context.
	if(!glfwInit()) {
		Log::Error() << Log::OpenGL << "Could not start GLFW3" << std::endl;
		return;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_VISIBLE, hidden ? GLFW_FALSE : GLFW_TRUE);
	glfwWindowHint(GLFW_FOCUSED, hidden ? GLFW_FALSE : GLFW_TRUE);
	
	if(config.fullscreen) {
		const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		/// \note We might want to impose the configured size here. This means the monitor could be set in a non-native mode.
		_window = glfwCreateWindow(mode->width, mode->height, name.c_str(), glfwGetPrimaryMonitor(), nullptr);
	} else {
		// Create a window with a given size. Width and height are defined in the configuration.
		_window = glfwCreateWindow(int(_config.initialWidth), int(_config.initialHeight), name.c_str(), nullptr, nullptr);
	}

	if(!_window) {
		Log::Error() << Log::OpenGL << "Could not open window with GLFW3" << std::endl;
		glfwTerminate();
		return;
	}

	if(_config.forceAspectRatio) {
		glfwSetWindowAspectRatio(_window, int(_config.initialWidth), int(_config.initialHeight));
	}
	// Bind the OpenGL context and the new window.
	glfwMakeContextCurrent(_window);

	if(gl3wInit()) {
		Log::Error() << Log::OpenGL << "Failed to initialize OpenGL" << std::endl;
		return;
	}
	if(!gl3wIsSupported(3, 2)) {
		Log::Error() << Log::OpenGL << "OpenGL 3.2 not supported\n"
					 << std::endl;
		return;
	}

	// Setup the GPU state.
	GLUtilities::setup();

	// Setup callbacks for various interactions and inputs.
	glfwSetFramebufferSizeCallback(_window, resize_callback);		  // Resizing the window
	glfwSetKeyCallback(_window, key_callback);						  // Pressing a key
	glfwSetCharCallback(_window, char_callback);						  // Outputing a text char (for ImGui)
	glfwSetMouseButtonCallback(_window, mouse_button_callback);		  // Clicking the mouse buttons
	glfwSetCursorPosCallback(_window, cursor_pos_callback);			  // Moving the cursor
	glfwSetScrollCallback(_window, scroll_callback);					  // Scrolling
	glfwSetJoystickCallback(joystick_callback);						  // Joystick
	glfwSetWindowIconifyCallback(_window, iconify_callback);			  // Window minimization
	glfwSwapInterval(_config.vsync ? (_config.rate == 30 ? 2 : 1) : 0); // 60 FPS V-sync

	setupImGui();

	// Check the window position and size (if we are on a screen smaller than the initial size).
	glfwGetWindowPos(_window, &_config.windowFrame[0], &_config.windowFrame[1]);
	glfwGetWindowSize(_window, &_config.windowFrame[2], &_config.windowFrame[3]);

	// On HiDPI screens, we have to consider the internal resolution for all framebuffers size.
	int width, height;
	glfwGetFramebufferSize(_window, &width, &height);
	config.screenResolution = glm::vec2(width, height);

	// Compute point density by computing the ratio.
	const float screenDensity = float(width) / float(_config.windowFrame[2]);
	Input::manager().densityEvent(screenDensity);

	// Update the resolution.
	Input::manager().resizeEvent(width, height);
}

void Window::perform(Action action) {
	switch(action) {
		case Action::Quit:
			glfwSetWindowShouldClose(_window, GLFW_TRUE);
			break;
		case Action::Vsync: {
			if(_config.vsync) {
				glfwSwapInterval(0);
			} else {
				glfwSwapInterval(_config.rate == 30 ? 2 : 1);
			}
			_config.vsync = !_config.vsync;
			break;
		}
		case Action::Fullscreen: {
			// Are we currently fullscreen?
			const bool fullscreen = glfwGetWindowMonitor(_window) != nullptr;

			if(fullscreen) {
				// Restore the window position and size.
				glfwSetWindowMonitor(_window, nullptr, _config.windowFrame[0], _config.windowFrame[1], _config.windowFrame[2], _config.windowFrame[3], 0);
				// Check the window position and size (if we are on a screen smaller than the initial size).
				glfwGetWindowPos(_window, &_config.windowFrame[0], &_config.windowFrame[1]);
				glfwGetWindowSize(_window, &_config.windowFrame[2], &_config.windowFrame[3]);
			} else {
				// Backup the window current frame.
				glfwGetWindowPos(_window, &_config.windowFrame[0], &_config.windowFrame[1]);
				glfwGetWindowSize(_window, &_config.windowFrame[2], &_config.windowFrame[3]);
				// Move to fullscreen on the primary monitor.
				GLFWmonitor * monitor	= glfwGetPrimaryMonitor();
				const GLFWvidmode * mode = glfwGetVideoMode(monitor);
				glfwSetWindowMonitor(_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			}
			_config.fullscreen = !fullscreen;
			// On some hardware, V-sync options can be lost.
			glfwSwapInterval(_config.vsync ? (_config.rate == 30 ? 2 : 1) : 0);

			// On HiDPI screens, we have to consider the internal resolution for all framebuffers size.
			int width, height;
			glfwGetFramebufferSize(_window, &width, &height);
			_config.screenResolution = glm::vec2(width, height);

			int wwidth, hheight;
			glfwGetWindowSize(_window, &wwidth, &hheight);
			// Compute point density by computing the ratio.
			const float screenDensity = float(width) / float(wwidth);
			Input::manager().densityEvent(screenDensity);

			// Update the resolution.
			Input::manager().resizeEvent(width, height);
		} break;
		default:
			break;
	}
}

bool Window::nextFrame() {
	if(_frameStarted){
		// Render the interface.
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		//Display the result for the current rendering loop.
		glfwSwapBuffers(_window);
	}
	// Update events (inputs,...).
	Input::manager().update();
	// Handle quitting.
	if(_allowEscape && Input::manager().pressed(Input::Key::Escape)) {
		perform(Action::Quit);
	}
	// Start new GUI frame.
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	_frameStarted = true;
	return !glfwWindowShouldClose(_window);
}

void Window::clean() {
	// Make sure rendering is done.
	if(_frameStarted){
		ImGui::EndFrame();
	}
	GLUtilities::sync();
	// Clean the interface.
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	// Close GL context and any other GLFW resources.
	glfwDestroyWindow(_window);
	glfwTerminate();
}

void Window::setupImGui() {
	ImGui::CreateContext();
	ImGuiIO & io = ImGui::GetIO();
	(void)io;
	ImGui_ImplGlfw_InitForOpenGL(_window, false);
	ImGui_ImplOpenGL3_Init("#version 150");
	
	// Customize the style.
	ImGui::StyleColorsDark();
	ImGuiStyle & style = ImGui::GetStyle();
	// Colors.
	ImVec4 * colors						  = style.Colors;
	colors[ImGuiCol_WindowBg]			  = ImVec4(0.04f, 0.04f, 0.04f, 0.94f);
	colors[ImGuiCol_FrameBg]			  = ImVec4(0.16f, 0.16f, 0.16f, 0.54f);
	colors[ImGuiCol_FrameBgHovered]		  = ImVec4(0.36f, 0.36f, 0.36f, 0.40f);
	colors[ImGuiCol_FrameBgActive]		  = ImVec4(0.54f, 0.54f, 0.54f, 0.67f);
	colors[ImGuiCol_TitleBgActive]		  = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab]		  = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_CheckMark]			  = ImVec4(0.84f, 0.84f, 0.84f, 1.00f);
	colors[ImGuiCol_SliderGrab]			  = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]	 = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
	colors[ImGuiCol_Button]				  = ImVec4(0.68f, 0.68f, 0.68f, 0.40f);
	colors[ImGuiCol_ButtonHovered]		  = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_ButtonActive]		  = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
	colors[ImGuiCol_Header]				  = ImVec4(0.57f, 0.57f, 0.57f, 0.31f);
	colors[ImGuiCol_HeaderHovered]		  = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
	colors[ImGuiCol_HeaderActive]		  = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_Separator]			  = ImVec4(0.41f, 0.41f, 0.41f, 0.50f);
	colors[ImGuiCol_SeparatorHovered]	 = ImVec4(0.43f, 0.43f, 0.43f, 0.78f);
	colors[ImGuiCol_SeparatorActive]	  = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
	colors[ImGuiCol_ResizeGrip]			  = ImVec4(1.00f, 1.00f, 1.00f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered]	= ImVec4(0.84f, 0.84f, 0.84f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]	 = ImVec4(0.88f, 0.88f, 0.88f, 0.95f);
	colors[ImGuiCol_PlotLinesHovered]	 = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
	colors[ImGuiCol_PlotHistogram]		  = ImVec4(0.62f, 0.62f, 0.62f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_TextSelectedBg]		  = ImVec4(0.67f, 0.67f, 0.67f, 0.35f);
	colors[ImGuiCol_DragDropTarget]		  = ImVec4(0.83f, 0.83f, 0.83f, 0.90f);
	colors[ImGuiCol_NavHighlight]		  = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
	// Frames.
	style.FrameRounding		 = 5.0f;
	style.GrabRounding		 = 3.0f;
	style.WindowRounding	 = 5.0f;
	style.ScrollbarRounding  = 12.0f;
	style.ScrollbarSize		 = 12.0f;
	style.WindowTitleAlign.x = 0.5f;
	style.FramePadding.y	 = 4.0f;
	style.ItemSpacing.y		 = 3.0f;
}
