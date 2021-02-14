#pragma once
#include "system/Config.hpp"
#include "Common.hpp"

struct GLFWwindow;

/** \brief Represent an OS window and its associated rendering context.
 \ingroup System
 */
class Window {
public:
	
	/** Create a new window backed by a GPU context.
	 \param name the name of the window
	 \param config the configuration to use (additional info will be added to it)
	 \param convertToSRGB should writes to the backbuffer be considered linear
	 \param escapeQuit allows the user to close the window by pressing escape
	 \param hidden should the window be hidden (for preprocess for instance)
	*/
	Window(const std::string & name, RenderingConfig & config, bool convertToSRGB, bool escapeQuit = true, bool hidden = false);

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
	 \return true if the next frame is valid.
	 \note If the frame is invalid, the window should be cleaned and closed.
	 */
	bool nextFrame();
	
	/** Copy constructor.*/
	Window(const Window &) = delete;
	
	/** Copy assignment.
	 \return a reference to the object assigned to
	 */
	Window & operator=(const Window &) = delete;
	
	/** Move constructor.*/
	Window(Window &&) = delete;
	
	/** Move assignment.
	 \return a reference to the object assigned to
	 */
	Window & operator=(Window &&) = delete;

	/** Destructor. Clean resources, delete the window and its context. */
	~Window();

private:
	/** Setup ImGui with the proper callbacks and style. */
	void setupImGui();
	
	RenderingConfig & _config; ///< The window configuration.
	GLFWwindow * _window = nullptr; ///< Internal window handle.
	bool _frameStarted = false; ///< Has a frame been started.
	bool _allowEscape = false; ///< Can the window be closed by pressing escape.
	bool _convertToSRGB = false; ///< Should writes to the backbuffer be considered linear.
};
