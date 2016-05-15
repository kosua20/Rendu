#include <stdio.h>
#include <iostream>
#include <vector>
#include <lodepng/lodepng.h>
// glm additional header to generate transformation matrices directly.
#include <glm/gtc/matrix_transform.hpp>

#include "helpers/ProgramUtilities.h"

#include "Renderer.h"

Renderer::Renderer(){}

Renderer::~Renderer(){}

void Renderer::init(int width, int height){
	_width = width;
	_height = height;

	updateProjectionMatrix();

	// initialize the timer
	_timer = glfwGetTime();


	// Query the renderer identifier, and the supported OpenGL version.
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	std::cout << "Renderer: " << renderer << std::endl;
	std::cout << "OpenGL version supported: " << version << std::endl;
	checkGLError();

	// GL options
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	checkGLError();
	
	// Load the shaders
	_programId = createGLProgram("ressources/shaders/prog2.vert","ressources/shaders/prog2.frag");

	// Create geometry : a cube, with position and color attributes.
	std::vector<float> cubeVertices{ -0.5, -0.5,  0.5,
									  0.5, -0.5,  0.5,
									 -0.5,  0.5,  0.5,
									  0.5,  0.5,  0.5,
									 -0.5, -0.5, -0.5,
									  0.5, -0.5, -0.5,
									 -0.5,  0.5, -0.5,
									  0.5,  0.5, -0.5
									};
	std::vector<float> cubeColors{ 	1.0, 0.0,  0.0,
									1.0, 1.0,  0.0,
								    0.5,  0.0,  0.5,
								    0.0,  1.0,  0.0,
							        1.0,  0.0, 0.5,
									0.0,  1.0,  1.0,
								    0.5,  0.1, 0.0,
									0.0,  0.0,  1.0
									};
	// Array to store the indices of the vertices to use.
	std::vector<unsigned int> cubeIndices{0, 1, 2, 2, 1, 3, // Front face
									  1, 5, 3, 3, 5, 7, // Right face
									  5, 4, 7, 7, 4, 6, // Back face
									  4, 0, 6, 6, 0, 2, // Left face
									  0, 4, 1, 1, 4, 5, // Bottom face
									  2, 3, 6, 6, 3, 7  // Top face  
	 };

	 _count = cubeIndices.size();

	// Create an array buffer to host the geometry data.
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// Upload the data to the Array buffer.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*cubeVertices.size(), &(cubeVertices[0]), GL_STATIC_DRAW);

	GLuint vbo_col = 0;
	glGenBuffers(1, &vbo_col);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_col);
	// Upload the data to the Array buffer.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*cubeColors.size(), &(cubeColors[0]), GL_STATIC_DRAW);

	// Generate a vertex array (useful when we add other attributes to the geometry).
	_vao = 0;
	glGenVertexArrays (1, &_vao);
	glBindVertexArray(_vao);
	// The first attribute will be the vertices positions.
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// The second attribute will be the colors.
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_col);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// We load the indices data
	glGenBuffers(1, &_ebo);
 	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
 	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * cubeIndices.size(), &(cubeIndices[0]), GL_STATIC_DRAW);

	glBindVertexArray(0);

}


void Renderer::draw(){

	// Compute the time elapsed since last frame
	float elapsed = glfwGetTime() - _timer;
	_timer = glfwGetTime();

	// Physics simulation
	physics(elapsed);

	// Scale the model by 0.5.
	glm::mat4 model = glm::scale(glm::mat4(1.0f),glm::vec3(0.5f));

	// Combine the three matrices.
	glm::mat4 MVP = _projection * _camera._view * model;

	// Set the clear color to white.
	glClearColor(1.0f,1.0f,1.0f,0.0f);
	// Clear the color and depth buffers.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Select the program (and shaders).
	glUseProgram(_programId);

	// Upload the MVP matrix.
	GLuint mvpID  = glGetUniformLocation(_programId, "mvp");
	glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);

	// Select the geometry.
	glBindVertexArray(_vao);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glDrawElements(GL_TRIANGLES, _count, GL_UNSIGNED_INT, (void*)0);

	// Update timer
	_timer = glfwGetTime();
}

void Renderer::physics(float elapsedTime){
	_camera.update(elapsedTime);
}


void Renderer::clean(){
	glDeleteVertexArrays(1, &_vao);
	glDeleteTextures(1, &_tex);
	glDeleteProgram(_programId);
}


void Renderer::resize(int width, int height){
	_width = width;
	_height = height;
	//Update the size of the viewport.
	glViewport(0, 0, width, height);
	updateProjectionMatrix();
}

void Renderer::keyPressed(int key, int action){
	if(action == GLFW_PRESS){
		if (key == GLFW_KEY_W || key == GLFW_KEY_A || key == GLFW_KEY_S || key == GLFW_KEY_D || key == GLFW_KEY_Q || key == GLFW_KEY_E){
			_camera.registerMove(key, true);

		} else if(key == GLFW_KEY_R) {
			_camera.reset();

		} else {
			std::cout << "Key: " << key << " (" << char(key) << ")." << std::endl;
		}
	} else if(action == GLFW_RELEASE) {
		if (key == GLFW_KEY_W || key == GLFW_KEY_A || key == GLFW_KEY_S || key == GLFW_KEY_D || key == GLFW_KEY_Q || key == GLFW_KEY_E){
			_camera.registerMove(key, false);
		}
	}
}

void Renderer::buttonPressed(int button, int action, double x, double y){
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			// We normalize the x and y values to the [-1, 1] range.
			float xPosition =  fmax(fmin(1.0f,2.0f * (float)x / _width - 1.0),-1.0f);
			float yPosition =  fmax(fmin(1.0f,2.0f * (float)y / _height - 1.0),-1.0f);
			_camera.startLeftMouse(xPosition, yPosition);
			return;
		} else if (action == GLFW_RELEASE) {
			_camera.endLeftMouse();
		}
	}
	std::cout << "Button: " << button << ", action: " << action << std::endl;            
}

void Renderer::mousePosition(int x, int y, bool leftPress, bool rightPress){
	if (leftPress){
		// We normalize the x and y values to the [-1, 1] range.
        float xPosition =  fmax(fmin(1.0f,2.0f * (float)x / _width - 1.0),-1.0f);
		float yPosition =  fmax(fmin(1.0f,2.0f * (float)y / _height - 1.0),-1.0f);
		_camera.leftMouseTo(xPosition, yPosition); 
    }
}

void Renderer::updateProjectionMatrix(){
	// Perspective projection.
	_projection = glm::perspective(45.0f, float(_width) / float(_height), 0.1f, 100.f);
}



