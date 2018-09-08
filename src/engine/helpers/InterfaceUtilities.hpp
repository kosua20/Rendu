#ifndef InterfaceUtilities_h
#define InterfaceUtilities_h

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

struct GLFWwindow;

/**
 \brief Performs interface setup, rendering and cleanup. Internally backed by ImGUI.
 \ingroup Helpers
 */
namespace ImGui {
	
	/** Setup the GUI, including interaction callbacks.
	 \param window the GLFW window
	 */
	void setup(GLFWwindow * window);
	
	/** Start registering GUI items. */
	void beginFrame();
	
	/** Finish registering GUI items and render them. */
	void endFrame();
	
	/** Clean internal resources. */
	void clean();
	
}

#endif
