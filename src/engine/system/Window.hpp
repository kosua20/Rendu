#pragma once
#include "system/Config.hpp"
#include "Common.hpp"
#include "graphics/GPUTypes.hpp"


struct GLFWwindow;
struct ImGui_ImplVulkan_InitInfo;
class Swapchain;

/** \brief Represent an OS window and its associated rendering context.
 \ingroup System
 */
class Window {
	friend class GPU;
	
public:
	
	/** Create a new window backed by a GPU context.
	 \param name the name of the window
	 \param config the configuration to use (additional info will be added to it)
	 \param escapeQuit allows the user to close the window by pressing escape
	 \param hidden should the window be hidden (for preprocess for instance)
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
	 \return true if the next frame is valid.
	 \note If the frame is invalid, the window should be cleaned and closed.
	 */
	bool nextFrame();
	
	/** Bind the backbuffer attachments
	 \param colorOp the operation for the color backbuffer
	 \param depthOp the operation for the depth backbuffer (if it exists)
	 \param stencilOp the operation for the stencil backbuffer (if it exists)
	 */
	void bind(const Load& colorOp, const Load& depthOp = Load::Operation::DONTCARE, const Load& stencilOp = Load::Operation::DONTCARE);

	/** Set the viewport to the window dimensions */
	void setViewport();

	/// \return the backbuffer color texture
	Texture& color();

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
	std::unique_ptr<Swapchain> _swapchain; ///< The swapchain displaying backbuffers.
	ImGui_ImplVulkan_InitInfo * _imgui = nullptr; ///< ImGui setup information.
	bool _frameStarted = false; ///< Has a frame been started.
	bool _allowEscape = false; ///< Can the window be closed by pressing escape.
};
