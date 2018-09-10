#ifndef InterfaceUtilities_h
#define InterfaceUtilities_h

#include "../Common.hpp"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

struct GLFWwindow;

/**
 \brief Performs interface setup, rendering, file picking. GUi is internally backed by ImGUI.
 \ingroup Helpers
 */
namespace Interface {
	
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
	
	/** The file picker mode. */
	enum PickerMode {
		Load, ///< Load an existing file.
		Directory, ///< open or create a directory.
		Save ///< Save to a new or existing file.
	};
	
	/** Present a filesystem document picker to the user, using native controls.
	 \param mode the type of item to ask to the user
	 \param startDir the initial directory when the picker is opended
	 \param outPath the path to the item selected by the user
	 \param extensions (optional) the extensions allowed, separated by "," or ";"
	 \return true if the user picked an item, false if cancelled.
	 */
	bool showPicker(const PickerMode mode, const std::string & startDir, std::string & outPath, const std::string & extensions = "");
	
}

#endif
