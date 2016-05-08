#include <GLFW/glfw3.h> // Contains GL functions prototypes.
#include <stdio.h>
#include <iostream>

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
}


void Renderer::draw(){
	// Set the clear color to white.
	glClearColor(1.0f,1.0f,1.0f,0.0f);
	// Clear the color and depth buffers.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
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



