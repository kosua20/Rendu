#include <stdio.h>
#include <iostream>
#include <vector>

#include "helpers/ProgramUtilities.h"

#include "Renderer.h"

Renderer::Renderer(){}

Renderer::~Renderer(){}

void Renderer::init(int width, int height){
	_width = width;
	_height = height;
	// Query the renderer identifier, and the supported OpenGL version.
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	std::cout << "Renderer: " << renderer << std::endl;
	std::cout << "OpenGL version supported: " << version << std::endl;
	checkGLError();

	// Load the shaders
	_programId = createGLProgram("ressources/shaders/prog1.vert","ressources/shaders/prog1.frag");

	// Create geometry : a plane covering the screen.
	std::vector<float> planeVertices{ -1.0, -1.0, 0.0,
									   1.0, -1.0, 0.0,
									  -1.0,  1.0, 0.0,
									  -1.0,  1.0, 0.0,
									   1.0, -1.0, 0.0,
									   1.0,  1.0, 0.0
									};
	// Create an array buffer to host the geometry data
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// Upload the data to the Array buffer.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*planeVertices.size(), &(planeVertices[0]), GL_STATIC_DRAW);

	// Generate a vertex array (useful when we will add other attributes to the geometry).
	_vao = 0;
	glGenVertexArrays (1, &_vao);
	glBindVertexArray(_vao);
	// The first attribute will be the vertices positions.
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindVertexArray(0);


}


void Renderer::draw(){
	// Set the clear color to white.
	glClearColor(1.0f,1.0f,1.0f,0.0f);
	// Clear the color and depth buffers.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Select the program (and shaders).
	glUseProgram(_programId);
	// Select the geometry.
	glBindVertexArray(_vao);
	// Draw!
	glDrawArrays(GL_TRIANGLES, 0, 2*3);
	
}


void Renderer::clean(){

}


void Renderer::resize(int width, int height){
	_width = width;
	_height = height;
	//Update the size of the viewport
	glViewport(0, 0, width, height);
}

void Renderer::keyPressed(int key, int action){
	std::cout << "Key: " << key << " (" << char(key) << "), action: " << action << "." << std::endl;
}

void Renderer::buttonPressed(int button, int action){
	std::cout << "Button: " << button << ", action: " << action << std::endl;
}



