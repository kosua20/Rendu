#include <stdio.h>
#include <iostream>
#include <vector>
#include <lodepng/lodepng.h>
// glm header, and additional header to generate transformation matrices directly.
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "helpers/ProgramUtilities.h"

#include "Renderer.h"

Renderer::Renderer(){}

Renderer::~Renderer(){}

void Renderer::init(int width, int height){
	_width = width;
	_height = height;

	// initialize the timer
	_timer = glfwGetTime();

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
	std::vector<float> planeUVs{ 0.0, 0.0,
								 1.0, 0.0,
								 0.0, 1.0,
								 0.0, 1.0,
								 1.0, 0.0,
								 1.0, 1.0,
								};
	// Create an array buffer to host the geometry data.
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// Upload the data to the Array buffer.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*planeVertices.size(), &(planeVertices[0]), GL_STATIC_DRAW);

	GLuint vbo_uv = 0;
	glGenBuffers(1, &vbo_uv);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_uv);
	// Upload the data to the Array buffer.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*planeUVs.size(), &(planeUVs[0]), GL_STATIC_DRAW);

	// Generate a vertex array (useful when we will add other attributes to the geometry).
	_vao = 0;
	glGenVertexArrays (1, &_vao);
	glBindVertexArray(_vao);
	// The first attribute will be the vertices positions.
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// The second attribute will be the uv coordinates.
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_uv);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindVertexArray(0);

	// Load and upload the texture.
	std::vector<unsigned char> image;
	unsigned imwidth, imheight;
  	unsigned error = lodepng::decode(image, imwidth, imheight, "ressources/grid.png");
  	if(error != 0){
  		std::cerr << "Unable to load the texture." << std::endl;
  		return;
  	}

  	flipImage(image,imwidth, imheight);
	// Active a texture slot and storage for the desired program.
	glUseProgram(_programId);
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &_tex);
	glBindTexture(GL_TEXTURE_2D, _tex);
	// Upload data to the GPU, with the correct settings: texture slot 0, pixel format, size, miplevel, internal format, type of each component, and pointer to the data. 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imwidth , imheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(image[0]));
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    // Setup a uniform to use the texture (as a sampler2D) in the shaders, through a handle. 
    GLuint texID  = glGetUniformLocation(_programId, "texture1");
	glUniform1i(texID, 0);

}


void Renderer::draw(){

	// Scale the model by 0.5.
	glm::mat4 model = glm::scale(glm::mat4(1.0f),glm::vec3(0.5f));
	// Translate it in (0.0,0.0,-1.0), and rotate it along its vertical axis, using the timer as an angle.
	glm::mat4 view = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,0.0f,-1.0f)), _timer, glm::vec3(0.0f,1.0f,0.0f));
	// Perspective projection.
	glm::mat4 projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.f);
	// Combine the three matrices.
	glm::mat4 MVP = projection * view * model;

	// Set the clear color to white.
	glClearColor(1.0f,1.0f,1.0f,0.0f);
	// Clear the color and depth buffers.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Select the program (and shaders).
	glUseProgram(_programId);

	// Bind the texture.
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _tex);

	// Compute the time elapsed since last frame
	float elapsed = glfwGetTime() - _timer;
	_timer = glfwGetTime();
	// Upload the time as a uniform
	GLuint timeID  = glGetUniformLocation(_programId, "time");
	glUniform1f(timeID, _timer);
	// Upload the MVP matrix.
	GLuint mvpID  = glGetUniformLocation(_programId, "mvp");
	glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);

	// Select the geometry.
	glBindVertexArray(_vao);
	// Draw!
	glDrawArrays(GL_TRIANGLES, 0, 2*3);
	
	// Update timer
	_timer = glfwGetTime();
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
}

void Renderer::keyPressed(int key, int action){
	std::cout << "Key: " << key << " (" << char(key) << "), action: " << action << "." << std::endl;
}

void Renderer::buttonPressed(int button, int action){
	std::cout << "Button: " << button << ", action: " << action << std::endl;
}



