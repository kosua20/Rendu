#pragma once
#include "system/Config.hpp"
#include "Common.hpp"

struct GLFWwindow;

class Window {
public:
	
	/** Create a new window backed by an OpenGL context.
	 \param name the name of the window
	 \param config the configuration to use (additional info will be added to it)
	 \param should the window be hidden (for preprocess for instance)
	*/
	Window(const std::string & name, RenderingConfig & config, bool escapeQuit = true, bool hidden = false);

	/** System actions that can be executed by the window. */
	enum class Action : uint {
		None,		///< Do nothing.
		Quit,		///< Quit the application.
		Fullscreen, ///< Switch the window from/to fullscreen mode.
		Vsync		///< Switch the v-sync on/off.
	};

	/** Execute an action related to the windowing system.
		 \param action the system action to perform
		 */
	void perform(Action action);
	
	/** Start registering GUI items.
	 \return true if the next frame is valid, else the window should be cleaned and destroyed.*/
	bool nextFrame();
	
	/** Clean resources, delete window. */
	void clean();
	
private:
	
	/** Setup ImGui with the proper callbacks and style. */
	void setupImGui();
	
	RenderingConfig & _config;
	GLFWwindow * _window = nullptr;
	bool _frameStarted = false;
	bool _allowEscape = false;
};
