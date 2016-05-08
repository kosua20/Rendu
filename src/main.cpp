#include <GL/glew.h> // to load OpenGL extensions at runtime
#include <GLFW/glfw3.h> // to set up the OpenGL context and manage window lifecycle and inputs

#include <stdio.h>
#include <iostream>

#define INITIAL_SIZE_WIDTH 800
#define INITIAL_SIZE_HEIGHT 600

/// Draw function

void draw(){
	// Set the clear color to white.
	glClearColor(1.0f,1.0f,1.0f,0.0f);
	// Clear the color and depth buffers.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
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

	// On OS X, GLEW needs the experimental flag, else some extensions won't be loaded.
	#ifdef __APPLE__
	glewExperimental = GL_TRUE;
	#endif
	// Initialize GLEW, for loading modern OpenGL extensions.
	glewInit();

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
	// Close GL context and any other GLFW resources.
	glfwTerminate();
	return 0;
}


