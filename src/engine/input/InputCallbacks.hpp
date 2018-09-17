#ifndef InputCallbacks_h
#define InputCallbacks_h

#include "../Common.hpp"

/**
 Callback for window resizing events.
 \param window the GLFW window pointer
 \param width the new width of the window
 \param height the new height of the window
 \ingroup Input
 */
void resize_callback(GLFWwindow* window, int width, int height);

/**
 Callback for key press/release events.
 \param window the GLFW window pointer
 \param key the key GLFW code
 \param scancode a platform-specific key code
 \param action the performed action GLFW code
 \param mods the applied modifiers (alt, ctrl,...) GLFW code
 \ingroup Input
 */
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

/**
 Callback for text characters.
 \param window the GLFW window pointer
 \param codepoint the character codepoint
 \ingroup Input
 */
void char_callback(GLFWwindow* window, unsigned int codepoint);

/**
 Callback for mouse buttons press/release events.
 \param window the GLFW window pointer
 \param button the button GLFW code
 \param action the performed action GLFW code
 \param mods the applied modifiers (alt, ctrl,...) GLFW code
 \ingroup Input
 */
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

/**
 Callback for cursor position events.
 \param window the GLFW window pointer
 \param xpos the horizontal position of the cursor, in pixels
 \param ypos the vertical position of the cursor, in pixels
 \note An event is received at each frame with the current mouse position.
 \ingroup Input
 */
void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);

/**
 Callback for mouse scroll events.
 \param window the GLFW window pointer
 \param xoffset the horizontal amount of scrolling
 \param yoffset the vertical amount of scrolling
 \note The scroll amounts are in arbitrary units.
 \ingroup Input
 */
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

/**
 Callback for window minimizing events.
 \param window the GLFW window pointer
 \param state the new state of the window
 \ingroup Input
 */
void iconify_callback(GLFWwindow* window, int state);

/**
 Callback for joystick (de)connection events.
 \param joy the index of the joystick
 \param event the event GLFW code
 \ingroup Input
 */
void joystick_callback(int joy, int event);

#endif
