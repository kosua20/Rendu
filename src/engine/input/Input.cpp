#include "input/Input.hpp"
#include "input/controller/GamepadController.hpp"
#include "input/controller/RawController.hpp"

#include <GLFW/glfw3.h>
#include <map>

// Singleton.
Input & Input::manager() {
	static Input * input = new Input();
	return *input;
}

Input::Input() {
	// Check if any joystick is available.
	bool first = true;
	for(int id = GLFW_JOYSTICK_1; id <= std::min(15, GLFW_JOYSTICK_LAST); ++id) {
		_controllers[id] = nullptr;
		// We only register the first joystick encountered if it exists.
		if(glfwJoystickPresent(id) == GL_TRUE && first) {
			joystickEvent(id, GLFW_CONNECTED);
			first			   = false;
			_joystickConnected = true;
		}
	}
}

void Input::preferRawControllers(bool prefer) {
	_preferRawControllers = prefer;

	for(int id = GLFW_JOYSTICK_1; id <= std::min(15, GLFW_JOYSTICK_LAST); ++id) {
		_controllers[id] = nullptr;
		// We only register the first joystick encountered if it exists.
		if(glfwJoystickPresent(id) == GL_TRUE) {
			joystickEvent(id, GLFW_CONNECTED);
			_joystickConnected = true;
		}
	}
}

void Input::keyPressedEvent(int key, int action) {
	
	if(key == GLFW_KEY_UNKNOWN) {
		return;
	}
	
	static const std::map<int, Key> glfwKeyToInternal = {
		{GLFW_KEY_SPACE, Key::Space},
		{GLFW_KEY_APOSTROPHE, Key::Apostrophe},
		{GLFW_KEY_COMMA, Key::Comma},
		{GLFW_KEY_MINUS, Key::Minus},
		{GLFW_KEY_PERIOD, Key::Period},
		{GLFW_KEY_SLASH, Key::Slash},
		{GLFW_KEY_0, Key::N0},
		{GLFW_KEY_1, Key::N1},
		{GLFW_KEY_2, Key::N2},
		{GLFW_KEY_3, Key::N3},
		{GLFW_KEY_4, Key::N4},
		{GLFW_KEY_5, Key::N5},
		{GLFW_KEY_6, Key::N6},
		{GLFW_KEY_7, Key::N7},
		{GLFW_KEY_8, Key::N8},
		{GLFW_KEY_9, Key::N9},
		{GLFW_KEY_SEMICOLON, Key::Semicolon},
		{GLFW_KEY_EQUAL, Key::Equal},
		{GLFW_KEY_A, Key::A},
		{GLFW_KEY_B, Key::B},
		{GLFW_KEY_C, Key::C},
		{GLFW_KEY_D, Key::D},
		{GLFW_KEY_E, Key::E},
		{GLFW_KEY_F, Key::F},
		{GLFW_KEY_G, Key::G},
		{GLFW_KEY_H, Key::H},
		{GLFW_KEY_I, Key::I},
		{GLFW_KEY_J, Key::J},
		{GLFW_KEY_K, Key::K},
		{GLFW_KEY_L, Key::L},
		{GLFW_KEY_M, Key::M},
		{GLFW_KEY_N, Key::N},
		{GLFW_KEY_O, Key::O},
		{GLFW_KEY_P, Key::P},
		{GLFW_KEY_Q, Key::Q},
		{GLFW_KEY_R, Key::R},
		{GLFW_KEY_S, Key::S},
		{GLFW_KEY_T, Key::T},
		{GLFW_KEY_U, Key::U},
		{GLFW_KEY_V, Key::V},
		{GLFW_KEY_W, Key::W},
		{GLFW_KEY_X, Key::X},
		{GLFW_KEY_Y, Key::Y},
		{GLFW_KEY_Z, Key::Z},
		{GLFW_KEY_LEFT_BRACKET, Key::LeftBracket},
		{GLFW_KEY_BACKSLASH, Key::Backslash},
		{GLFW_KEY_RIGHT_BRACKET, Key::RightBracket},
		{GLFW_KEY_GRAVE_ACCENT, Key::GraveAccent},
		{GLFW_KEY_WORLD_1, Key::World1},
		{GLFW_KEY_WORLD_2, Key::World2},
		{GLFW_KEY_ESCAPE, Key::Escape},
		{GLFW_KEY_ENTER, Key::Enter},
		{GLFW_KEY_TAB, Key::Tab},
		{GLFW_KEY_BACKSPACE, Key::Backspace},
		{GLFW_KEY_INSERT, Key::Insert},
		{GLFW_KEY_DELETE, Key::Delete},
		{GLFW_KEY_RIGHT, Key::Right},
		{GLFW_KEY_LEFT, Key::Left},
		{GLFW_KEY_DOWN, Key::Down},
		{GLFW_KEY_UP, Key::Up},
		{GLFW_KEY_PAGE_UP, Key::PageUp},
		{GLFW_KEY_PAGE_DOWN, Key::PageDown},
		{GLFW_KEY_HOME, Key::Home},
		{GLFW_KEY_END, Key::End},
		{GLFW_KEY_CAPS_LOCK, Key::CapsLock},
		{GLFW_KEY_SCROLL_LOCK, Key::ScrollLock},
		{GLFW_KEY_NUM_LOCK, Key::NumLock},
		{GLFW_KEY_PRINT_SCREEN, Key::PrintScreen},
		{GLFW_KEY_PAUSE, Key::Pause},
		{GLFW_KEY_F1, Key::F1},
		{GLFW_KEY_F2, Key::F2},
		{GLFW_KEY_F3, Key::F3},
		{GLFW_KEY_F4, Key::F4},
		{GLFW_KEY_F5, Key::F5},
		{GLFW_KEY_F6, Key::F6},
		{GLFW_KEY_F7, Key::F7},
		{GLFW_KEY_F8, Key::F8},
		{GLFW_KEY_F9, Key::F9},
		{GLFW_KEY_F10, Key::F10},
		{GLFW_KEY_F11, Key::F11},
		{GLFW_KEY_F12, Key::F12},
		{GLFW_KEY_F13, Key::F13},
		{GLFW_KEY_F14, Key::F14},
		{GLFW_KEY_F15, Key::F15},
		{GLFW_KEY_F16, Key::F16},
		{GLFW_KEY_F17, Key::F17},
		{GLFW_KEY_F18, Key::F18},
		{GLFW_KEY_F19, Key::F19},
		{GLFW_KEY_F20, Key::F20},
		{GLFW_KEY_F21, Key::F21},
		{GLFW_KEY_F22, Key::F22},
		{GLFW_KEY_F23, Key::F23},
		{GLFW_KEY_F24, Key::F24},
		{GLFW_KEY_F25, Key::F25},
		{GLFW_KEY_KP_0, Key::Pad0},
		{GLFW_KEY_KP_1, Key::Pad1},
		{GLFW_KEY_KP_2, Key::Pad2},
		{GLFW_KEY_KP_3, Key::Pad3},
		{GLFW_KEY_KP_4, Key::Pad4},
		{GLFW_KEY_KP_5, Key::Pad5},
		{GLFW_KEY_KP_6, Key::Pad6},
		{GLFW_KEY_KP_7, Key::Pad7},
		{GLFW_KEY_KP_8, Key::Pad8},
		{GLFW_KEY_KP_9, Key::Pad9},
		{GLFW_KEY_KP_DECIMAL, Key::PadDecimal},
		{GLFW_KEY_KP_DIVIDE, Key::PadDivide},
		{GLFW_KEY_KP_MULTIPLY, Key::PadMultiply},
		{GLFW_KEY_KP_SUBTRACT, Key::PadSubtract},
		{GLFW_KEY_KP_ADD, Key::PadAdd},
		{GLFW_KEY_KP_ENTER, Key::PadEnter},
		{GLFW_KEY_KP_EQUAL, Key::PadEqual},
		{GLFW_KEY_LEFT_SHIFT, Key::LeftShift},
		{GLFW_KEY_LEFT_CONTROL, Key::LeftControl},
		{GLFW_KEY_LEFT_ALT, Key::LeftAlt},
		{GLFW_KEY_LEFT_SUPER, Key::LeftSuper},
		{GLFW_KEY_RIGHT_SHIFT, Key::RightShift},
		{GLFW_KEY_RIGHT_CONTROL, Key::RightControl},
		{GLFW_KEY_RIGHT_ALT, Key::RightAlt},
		{GLFW_KEY_RIGHT_SUPER, Key::RightSuper},
		{GLFW_KEY_MENU, Key::Menu}
	};
	
	const uint keycode = uint(glfwKeyToInternal.at(key));
	if(action == GLFW_PRESS) {
		_keys[keycode].pressed = true;
		_keys[keycode].first   = true;
		_keys[keycode].last	= false;
	} else if(action == GLFW_RELEASE) {
		_keys[keycode].pressed = false;
		_keys[keycode].first   = false;
		_keys[keycode].last	= true;
	}
	_keyInteracted = true;

	Log::Verbose() << Log::Input << "Key " << key << ", " << (action == GLFW_PRESS ? "pressed" : (action == GLFW_RELEASE ? "released" : "held")) << "." << std::endl;
}

void Input::joystickEvent(int joy, int event) {

	if(event == GLFW_CONNECTED) {

		Log::Verbose() << Log::Input << "Joystick: connected joystick " << joy << "." << std::endl;

		// Register the new one, if no joystick was activated consider this one as the active one.
		if(!_controllers[joy]) {

			if(!_preferRawControllers && glfwJoystickIsGamepad(joy)) {
				// If this is a gamepad, GLFW has a mapping, all is good.
				_controllers[joy] = std::unique_ptr<Controller>(new GamepadController());
			} else {
				// Fallback on a raw controller.
				_controllers[joy] = std::unique_ptr<Controller>(new RawController());
			}
		}
		// Ignore non-configured controllers.
		if(!_controllers[joy]) {
			return;
		}
		const bool res = _controllers[joy]->activate(joy);
		if(res && _activeController == -1) {
			_activeController  = joy;
			_joystickConnected = true;
		}

	} else if(event == GLFW_DISCONNECTED) {

		Log::Verbose() << Log::Input << "Joystick: disconnected joystick " << joy << "." << std::endl;

		// If the disconnected joystick is the one currently used, register this.
		if(_controllers[joy]) {
			_controllers[joy]->deactivate();
			if(joy == _activeController) {
				_activeController	 = -1;
				_joystickDisconnected = true;
			}
		}
		// Here we could also try to fall back on any other (still) connected joystick.
	}
}

void Input::mousePressedEvent(int button, int action) {
	
	static const std::map<int, Mouse> glfwMouseToInternal = {
		{GLFW_MOUSE_BUTTON_LEFT  , Mouse::Left},
		{GLFW_MOUSE_BUTTON_RIGHT , Mouse::Right},
		{GLFW_MOUSE_BUTTON_MIDDLE, Mouse::Middle}
	};
	
	const uint mouse = uint(glfwMouseToInternal.at(button));
	if(action == GLFW_PRESS) {
		_mouseButtons[mouse].pressed = true;
		_mouseButtons[mouse].first   = true;
		_mouseButtons[mouse].last	= false;
		_mouseButtons[mouse].x0	  = _mouse.x;
		_mouseButtons[mouse].y0	  = _mouse.y;
		// Delta = 0 at the beginning.
		_mouseButtons[mouse].x1 = _mouse.x;
		_mouseButtons[mouse].y1 = _mouse.y;

	} else if(action == GLFW_RELEASE) {
		_mouseButtons[mouse].pressed = false;
		_mouseButtons[mouse].first   = false;
		_mouseButtons[mouse].last	= true;
		_mouseButtons[mouse].x1	  = _mouse.x;
		_mouseButtons[mouse].y1	  = _mouse.y;
	}
	_mouseInteracted = true;
	Log::Verbose() << Log::Input << "Mouse pressed: " << button << ", " << action << " at " << _mouse.x << "," << _mouse.y << "." << std::endl;
}

void Input::mouseMovedEvent(double x, double y) {
	_mouse.x = x / _width * _density;
	_mouse.y = y / _height * _density;
	Log::Verbose() << Log::Input << "Mouse moved: " << x << "," << y << " (" << _mouse.x << "," << _mouse.y << ")." << std::endl;
}

void Input::mouseScrolledEvent(double xoffset, double yoffset) {
	_mouse.scroll = glm::vec2(xoffset, yoffset);
	Log::Verbose() << Log::Input << "Mouse scrolled: " << xoffset << "," << yoffset << "." << std::endl;
	_mouseInteracted = (xoffset != 0.0f || yoffset != 0.0f);
}

void Input::resizeEvent(int width, int height) {
	_width   = width > 0 ? width : 1;
	_height  = height > 0 ? height : 1;
	_resized = true;
	Log::Verbose() << Log::Input << "Resize event: " << width << "," << height << "." << std::endl;
	_windowInteracted = true;
}

void Input::minimizedEvent(bool minimized) {
	_minimized		  = minimized;
	_windowInteracted = true;
}

void Input::densityEvent(float density) {
	_density = density;
}

void Input::update() {
	if(_minimized) {
		glfwWaitEvents();
	}

	// Reset temporary state (first, last).
	for(auto & key : _keys) {
		key.first = false;
		key.last  = false;
	}
	for(auto & button : _mouseButtons) {
		button.first = false;
		button.last  = false;
	}
	_mouse.scroll = glm::vec2(0.0f, 0.0f);
	_resized	  = false;

	_mouseInteracted  = false;
	_keyInteracted	= false;
	_windowInteracted = false;

	// Update only the active joystick if it exists.
	_joystickConnected	= false;
	_joystickDisconnected = false;
	if(_activeController >= 0) {
		_controllers[_activeController]->update();
	}

	glfwPollEvents();
}

bool Input::pressed(const Key & keyboardKey) const {
	return _keys[uint(keyboardKey)].pressed;
}

bool Input::triggered(const Key & keyboardKey, bool absorb) {
	const bool res = _keys[uint(keyboardKey)].first;
	if(absorb) {
		_keys[uint(keyboardKey)].first = false;
	}
	return res;
}

bool Input::released(const Key & keyboardKey, bool absorb) {
	const bool res = _keys[uint(keyboardKey)].last;
	if(absorb) {
		_keys[uint(keyboardKey)].last = false;
	}
	return res;
}

bool Input::pressed(const Mouse & mouseButton) const {
	return _mouseButtons[uint(mouseButton)].pressed;
}

bool Input::triggered(const Mouse & mouseButton, bool absorb) {
	const bool res = _mouseButtons[uint(mouseButton)].first;
	if(absorb) {
		_mouseButtons[uint(mouseButton)].first = false;
	}
	return res;
}

bool Input::released(const Mouse & mouseButton, bool absorb) {
	const bool res = _mouseButtons[uint(mouseButton)].last;
	if(absorb) {
		_mouseButtons[uint(mouseButton)].last = false;
	}
	return res;
}

glm::vec2 Input::mouse(bool inFramebuffer) const {
	if(inFramebuffer) {
		const glm::vec2 mousePosition = glm::floor(glm::vec2(_mouse.x * _width, (1.0f - _mouse.y) * _height));
		return glm::clamp(mousePosition, glm::vec2(0.0f), glm::vec2(_width, _height));
	}
	return glm::vec2(_mouse.x, _mouse.y);
}

glm::vec2 Input::moved(const Mouse & mouseButton) const {
	const MouseButton & b = _mouseButtons[uint(mouseButton)];
	if(b.pressed) {
		return glm::vec2(_mouse.x - b.x0, _mouse.y - b.y0);
	}
	return glm::vec2(0.0f, 0.0f);
}

glm::vec2 Input::scroll() const {
	return _mouse.scroll;
}

float Input::density() const {
	return _density;
}

bool Input::interacted() const {
	return _keyInteracted || _mouseInteracted || _windowInteracted;
}
