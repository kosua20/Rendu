#include <GL/glew.h> // to load OpenGL extensions at runtime
#include <GLFW/glfw3.h> // to set up the OpenGL context and manage window lifecycle and inputs

#include <stdio.h>
#include <iostream>

#define INITIAL_SIZE_WIDTH 800
#define INITIAL_SIZE_HEIGHT 600

/// Init function

void init(){
	// Query the renderer identifier, and the supported OpenGL version.
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	std::cout << "Renderer: " << renderer << std::endl;
	std::cout << "OpenGL version supported: " << version << std::endl;
}


/// Draw function

void draw(){
	// Set the clear color to white.
	glClearColor(1.0f,1.0f,1.0f,0.0f);
	// Clear the color and depth buffers.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
}


/// Clean function

void clean(){

}


/// Callbacks

/// Window resize callback

static void resize_callback(GLFWwindow* window, int width, int height){
	//Update the size of the viewport
	glViewport(0, 0, width, height);
}

/// Key pressed callback

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	// Handle quitting
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){ 
		glfwSetWindowShouldClose(window, GL_TRUE);
		return;
	} 
	std::cout << "Key: " << key << " (" << char(key) << "), action: " << action << ", modes: " << mods << std::endl;	
}

/// Mouse button pressed callback

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
	std::cout << "Button: " << button << ", action: " << action << std::endl;
}

/// Cursor position callback

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos){
	// Do nothing for now
	// ...
}

/// Scrolling callback

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
	// Do nothing for now
	// ...
}

/// The main function

int main () {
	// Initialize glfw, which will create and setup an OpenGL context.
	if (!glfwInit()) {
		std::cerr << "ERROR: could not start GLFW3" << std::endl;
		return 1;
	}

	// On OS X, the correct OpenGL profile and version to use have to be explicitely defined.
	#ifdef __APPLE__
	glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	#endif

	// Create a window with a given size. Width and height are macros as we will need them again.
	GLFWwindow* window = glfwCreateWindow(INITIAL_SIZE_WIDTH, INITIAL_SIZE_HEIGHT,"GL_Template", NULL, NULL);
	if (!window) {
		std::cerr << "ERROR: could not open window with GLFW3" << std::endl;
		glfwTerminate();
		return 1;
	}

	// Bind the OpenGL context and the new window.
	glfwMakeContextCurrent(window);

	// Setup callbacks for various interactions and inputs
	glfwSetFramebufferSizeCallback(window, resize_callback);	// Resizing the window
	glfwSetKeyCallback(window,key_callback);					// Pressing a key
	glfwSetMouseButtonCallback(window,mouse_button_callback);	// Clicking the mouse buttons
	glfwSetCursorPosCallback(window,cursor_pos_callback);		// Moving the cursor
	glfwSetScrollCallback(window,scroll_callback);				// Scrolling
	

	// On OS X, GLEW needs the experimental flag, else some extensions won't be loaded.
	#ifdef __APPLE__
	glewExperimental = GL_TRUE;
	#endif
	// Initialize GLEW, for loading modern OpenGL extensions.
	glewInit();

	// Initialization function
	init();

	// Start the display/interaction loop.
	while (!glfwWindowShouldClose(window)) {

		// Update the content of the window.
		draw();
		
		//Display the result fo the current rendering loop.
		glfwSwapBuffers(window);

		// Update events (inputs,...).
		glfwPollEvents();
	}

	// Remove the window.
	glfwDestroyWindow(window);
	// Clean other ressources
	clean();
	// Close GL context and any other GLFW resources.
	glfwTerminate();
	return 0;
}


