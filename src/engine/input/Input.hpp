#pragma once

#include "input/controller/Controller.hpp"
#include "Common.hpp"

/**
 \brief The input manager is responsible for updating the internal input states (keyboard, mouse, window size). It can also be use to query back these states.
 \ingroup Input
 */
class Input {
public:
	/// Keys codes.
	enum class Key : uint {
		Space = 0, Apostrophe, Comma, Minus, Period, Slash,
		N0, N1, N2, N3, N4, N5, N6, N7, N8, N9, Semicolon, Equal,
		A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S,
		T, U, V, W, X, Y, Z, LeftBracket, Backslash, RightBracket,
		GraveAccent, World1, World2, Escape, Enter, Tab, Backspace,
		Insert, Delete, Right, Left, Down, Up, PageUp, PageDown,
		Home, End, CapsLock, ScrollLock, NumLock, PrintScreen, Pause,
		F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14,
		F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25,
		Pad0, Pad1, Pad2, Pad3, Pad4, Pad5, Pad6, Pad7, Pad8, Pad9,
		PadDecimal, PadDivide, PadMultiply, PadSubtract, PadAdd,
		PadEnter, PadEqual, LeftShift, LeftControl, LeftAlt, LeftSuper,
		RightShift, RightControl, RightAlt, RightSuper, Menu, Count
	};

	/// Mouse buttons codes.
	enum class Mouse : uint {
		Left   = 0,
		Right  = 1,
		Middle = 2,
		Count  = 3
	};

	/**
	 \name Input settings
	 @{*/

	/** Disable the GLFW/SDL available controller mappings.
	 \param prefer should the input only use raw controllers
	 */
	void preferRawControllers(bool prefer);

	/**@}*/

	/**
	 \name Input updates
	 @{*/

	/**
	 Register a keyboard event.
	 \param key the key code
	 \param action the action code (pressed/released)
	 */
	void keyPressedEvent(int key, int action);

	/**
	 Register a joystick event.
	 \param joy the ID of the joystick
	 \param event the joystick event
	 */
	void joystickEvent(int joy, int event);

	/**
	 Register a mouse button event.
	 \param button the button code
	 \param action the action code (pressed/released)
	 */
	void mousePressedEvent(int button, int action);

	/**
	 Register a mouse move event.
	 \param x the horizontal position of the cursor, in pixels
	 \param y the vertical position of the cursor, in pixels
	 */
	void mouseMovedEvent(double x, double y);

	/**
	 Register a mouse scroll event.
	 \param xoffset the horizontal amount of scrolling
	 \param yoffset the vertical amount of scrolling
	 */
	void mouseScrolledEvent(double xoffset, double yoffset);

	/**
	 Register a window size change event.
	 \param width the new width of the window
	 \param height the new height of the window
	 */
	void resizeEvent(int width, int height);

	/**
	 Register a window minification event.
	 \param minimized the current state of the window
	 */
	void minimizedEvent(bool minimized);

	/**
	 Register a screen density change event.
	 \param density the new density of the screen
	 */
	void densityEvent(float density);

	/**
	 Trigger an update of the internal state
	 */
	void update();

	/**@}*/

	/**
	 \name Input queries
	 @{*/

	/**
	 Query if the window has been resized at this frame.
	 \return true if the window was resized
	 */
	bool resized() const { return _resized; }

	/**
	 Query if the window has been minimized.
	 \return true if the window is minimized
	 */
	bool minimized() const { return _minimized; }

	/**
	 Query the current window size.
	 \return the size of the window, in pixels
	 */
	glm::ivec2 size() const { return glm::ivec2(_width, _height); }

	/**
	 Query if a controller (joystick) is available.
	 \return true if a controller is available
	 */
	bool controllerAvailable() const { return _activeController >= 0; }

	/**
	 Query if a controller (joystick) was connected at this frame.
	 \return true if a controller has just been connected
	 */
	bool controllerConnected() const { return _joystickConnected; }

	/**
	 Query if a controller (joystick) was disconnected at this frame.
	 \return true if a controller has just been disconnected
	 */
	bool controllerDisconnected() const { return _joystickDisconnected; }

	/**
	 Query the current controller (joystick).
	 \return a reference to the current controller
	 \warning Make sure a controller is available before calling this method.
	 */
	Controller * controller() const { return _controllers[_activeController].get(); }

	/**
	 Query if a given key is held at this frame.
	 \param keyboardKey the key code
	 \return true if the key is currently pressed.
	 */
	bool pressed(const Key & keyboardKey) const;

	/**
	 Query if a given key was pressed at this frame precisely.
	 \param keyboardKey the key code
	 \param absorb should the press event be hidden from future queries during the current frame
	 \return true if the key was triggered at this frame.
	 */
	bool triggered(const Key & keyboardKey, bool absorb = false);

	/**
	 Query if a given key was released at this frame precisely.
	 \param keyboardKey the key code
	 \param absorb should the press event be hidden from future queries during the current frame
	 \return true if the key was released at this frame.
	 */
	bool released(const Key & keyboardKey, bool absorb = false);

	/**
	 Query if a given mouse button is held at this frame.
	 \param mouseButton the mouse button code
	 \return true if the button is currently pressed.
	 */
	bool pressed(const Mouse & mouseButton) const;

	/**
	 Query if a given mouse button was pressed at this frame precisely.
	 \param mouseButton the mouse button code
	 \param absorb should the press event be hidden from future queries during the current frame
	 \return true if the mouse button was triggered at this frame.
	 */
	bool triggered(const Mouse & mouseButton, bool absorb = false);

	/**
	 Query if a given mouse button was released at this frame precisely.
	 \param mouseButton the mouse button code
	 \param absorb should the press event be hidden from future queries during the current frame
	 \return true if the mouse button was released at this frame.
	 */
	bool released(const Mouse & mouseButton, bool absorb = false);

	/**
	 Query the current mouse position.
	 \param inFramebuffer should the position be expressed in an OpenGL compatible fashion
	 \return the current mouse position in unit coordinates or pixels
	 \note The mouse position will be expressed by default in the [0,1] range, from the bottom left corner. If inFramebuffer is set to true, the position will be expressed in pixels, from the top left corner, clamped to the window size.
	 */
	glm::vec2 mouse(bool inFramebuffer = false) const;

	/**
	 Query the amount of cursor displacement since a given mouse button started to be held. If the button is not currently pressed, is returns a null displacement.
	 \param mouseButton the mouse button to track
	 \return the amount of displacement on both axis in unit coordinates.
	 \note The displacement will be expressed in the [0,1] range.
	 */
	glm::vec2 moved(const Mouse & mouseButton) const;

	/**
	 Query the current scroll amount.
	 \return the current scrolling amount on both axis
	 \note The scroll amount is in arbitrary units.
	 */
	glm::vec2 scroll() const;

	/** Query the current screen density.
	 \return The screen density.
	 */
	float density() const;

	/** Check if the user interacted with the keyboard, window, or mouse (except mouse moves).
	 \return true if any interaction happened.
	 */
	bool interacted() const;

	/**@}*/

private:
	// State.

	// Resize state.
	unsigned int _width  = 1;	 ///< Internal window width in pixels.
	unsigned int _height = 1;	 ///< Internal window height in pixels.
	bool _resized		 = false; ///< Denote if the window was resized at the current frame.
	bool _minimized		 = false; ///< Is the window minimized and thus hidden.
	float _density		 = 1.0f;  ///< The screen density.

	// Joystick state.
	int _activeController = -1;										  ///< The active joystick ID, or -1 if no controller active.
	std::unique_ptr<Controller> _controllers[16]; ///< States of all possible controllers.
	bool _preferRawControllers = false;
	bool _joystickConnected	= false;
	bool _joystickDisconnected = false;

	/// Mouse state.
	struct MouseButton {
		double x0	= 0.0;   ///< Horizontal coordinate at the beginning of the last press.
		double y0	= 0.0;   ///< Vertical coordinate at the beginning of the last press.
		double x1	= 0.0;   ///< Horizontal coordinate at the end of the last press.
		double y1	= 0.0;   ///< Vertical coordinate at the end of the last press.
		bool pressed = false; ///< Is the button currently held.
		bool first   = false; ///< Is it the first frame it is held.
		bool last	= false; ///< Is is the first frame since it was released.
	};
	MouseButton _mouseButtons[uint(Mouse::Count)]; ///< States of all possible mouse buttons.

	/// Mouse cursor state.
	struct MouseCursor {
		double x		 = 0.0;			   ///< Current cursor horizontal position.
		double y		 = 0.0;			   ///< Current cursor vertical position.
		glm::vec2 scroll = glm::vec2(0.0); ///< Current amount of scroll.
	};
	MouseCursor _mouse; ///< State of the mouse cursor.

	/// Keyboard state.
	struct KeyboardKey {
		bool pressed = false; ///< Is the key currently held.
		bool first   = false; ///< Is it the first frame it is held.
		bool last	= false; ///< Is is the first since frame it was released.
	};
	KeyboardKey _keys[uint(Key::Count)]; ///< States of all possible keyboard keys.

	bool _mouseInteracted  = false; ///< Did the user interact with the mouse.
	bool _keyInteracted	= false; ///< Did the user interact with the keyboard.
	bool _windowInteracted = false; ///< Did the user interact with the window (minimize, resize,...).

	// Singleton management.

public:
	/**
	 Accessor to the Input manager singleton.
	 \return the Input manager
	 */
	static Input & manager();

	/** Copy operator (disabled).
	 \return a reference to the object assigned to
	 */
	Input & operator=(const Input &) = delete;

	/** Copy constructor (disabled). */
	Input(const Input &) = delete;

	/** Move operator (disabled).
	 \return a reference to the object assigned to
	 */
	Input & operator=(Input &&) = delete;

	/** Move constructor (disabled). */
	Input(Input &&) = delete;

private:
	/// Constructor (disabled).
	Input();

	/// Destructor (disabled).
	~Input() = default;
};
