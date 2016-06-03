#include <stdio.h>
#include <iostream>
#include <vector>
#include <lodepng/lodepng.h>
#include <glm/gtc/matrix_transform.hpp>

#include "helpers/ProgramUtilities.h"

#include "Cube.h"

Cube::Cube(){}

Cube::~Cube(){}

void Cube::init(){
	
	// Load the shaders
	_programId = createGLProgram("ressources/shaders/cube.vert","ressources/shaders/cube.frag");

	// Load geometry.
	std::vector<float> cubeVertices{ -1.0, -1.0,  1.0,
		1.0, -1.0,  1.0,
		-1.0,  1.0,  1.0,
		1.0,  1.0,  1.0,
		-1.0, -1.0, -1.0,
		1.0, -1.0, -1.0,
		-1.0,  1.0, -1.0,
		1.0,  1.0, -1.0
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
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * cubeVertices.size(), &(cubeVertices[0]), GL_STATIC_DRAW);


	// Generate a vertex array (useful when we add other attributes to the geometry).
	_vao = 0;
	glGenVertexArrays (1, &_vao);
	glBindVertexArray(_vao);
	// The first attribute will be the vertices positions.
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	

	// We load the indices data
	glGenBuffers(1, &_ebo);
 	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
 	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * cubeIndices.size(), &(cubeIndices[0]), GL_STATIC_DRAW);

	glBindVertexArray(0);
		
	checkGLError();
	
}


void Cube::draw(float elapsed, const glm::mat4& view, const glm::mat4& projection){
	
	glm::mat4 model = glm::scale(glm::mat4(1.0f),glm::vec3(0.25f));
	// Combine the three matrices.
	glm::mat4 MV = view * model;
	glm::mat4 MVP = projection * MV;
	
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

	checkGLError();
	glBindVertexArray(0);
	glUseProgram(0);
}


void Cube::clean(){
	glDeleteVertexArrays(1, &_vao);
	glDeleteTextures(1, &_texCubeMap);
}


