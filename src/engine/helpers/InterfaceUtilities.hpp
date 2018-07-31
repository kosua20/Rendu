#ifndef InterfaceUtilities_h
#define InterfaceUtilities_h

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

struct GLFWwindow;

namespace ImGui {
	
	void setup(GLFWwindow * window);
	void beginFrame();
	void endFrame();
	void clean();
	
}

#endif
