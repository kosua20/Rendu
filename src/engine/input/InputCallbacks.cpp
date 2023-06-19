#include "input/InputCallbacks.hpp"
#include "input/Input.hpp"
#include "system/System.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <GLFW/glfw3.h>

void resize_callback(GLFWwindow *, int width, int height) {
	Input::manager().resizeEvent(width, height);
}

void key_callback(GLFWwindow * window, int key, int scancode, int action, int mods) {
	if(!ImGui::GetIO().WantCaptureKeyboard) {
		Input::manager().keyPressedEvent(key, action);
	}
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

void char_callback(GLFWwindow * window, unsigned int codepoint) {
	ImGui_ImplGlfw_CharCallback(window, codepoint);
}

void mouse_button_callback(GLFWwindow * window, int button, int action, int mods) {
	if(!ImGui::GetIO().WantCaptureMouse) {
		Input::manager().mousePressedEvent(button, action);
	}
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

void cursor_pos_callback(GLFWwindow * window, double xpos, double ypos) {
	if(!ImGui::GetIO().WantCaptureMouse) {
		Input::manager().mouseMovedEvent(xpos, ypos);
	}
	ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
}

void scroll_callback(GLFWwindow * window, double xoffset, double yoffset) {
	if(!ImGui::GetIO().WantCaptureMouse) {
		Input::manager().mouseScrolledEvent(xoffset, yoffset);
	}
	ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

void iconify_callback(GLFWwindow *, int state) {
	Input::manager().minimizedEvent(state == GLFW_TRUE);
}

void joystick_callback(int joy, int event) {
	Input::manager().joystickEvent(joy, event);
}
