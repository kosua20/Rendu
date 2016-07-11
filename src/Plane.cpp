#include <stdio.h>
#include <iostream>
#include <vector>
#include <lodepng/lodepng.h>
#include <glm/gtc/matrix_transform.hpp>

#include "helpers/ProgramUtilities.h"

#include "Plane.h"

Plane::Plane(){}

Plane::~Plane(){}

void Plane::init(){
	
	// Load the shaders
	_programDepthId = createGLProgram("ressources/shaders/plane_depth.vert","ressources/shaders/plane_depth.frag");
	_programId = createGLProgram("ressources/shaders/plane.vert","ressources/shaders/plane.frag");

	// Load geometry.
	std::vector<float> cubeVertices{ -1.0, 0.0,  -1.0,
		1.0, 0.0,  -1.0,
		-1.0, 0.0, 1.0,
		1.0, 0.0, 1.0
	};
	
	std::vector<float> cubeNormals{ 0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0
	};

	// Array to store the indices of the vertices to use.
	std::vector<unsigned int> cubeIndices{ 0, 2, 1, 1, 2, 3 };
	
	_count = cubeIndices.size();

	// Create an array buffer to host the geometry data.
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// Upload the data to the Array buffer.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * cubeVertices.size(), &(cubeVertices[0]), GL_STATIC_DRAW);

	GLuint vbo_nor = 0;
	glGenBuffers(1, &vbo_nor);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_nor);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * cubeNormals.size() * 3, &(cubeNormals[0]), GL_STATIC_DRAW);
	
	
	// Generate a vertex array (useful when we add other attributes to the geometry).
	_vao = 0;
	glGenVertexArrays (1, &_vao);
	glBindVertexArray(_vao);
	// The first attribute will be the vertices positions.
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	// The second attribute will be the vertices normals.
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_nor);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	
	// We load the indices data
	glGenBuffers(1, &_ebo);
 	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
 	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * cubeIndices.size(), &(cubeIndices[0]), GL_STATIC_DRAW);

	glBindVertexArray(0);
	
	// Get a binding point for the light in Uniform buffer.
	_lightUniformId = glGetUniformBlockIndex(_programId, "Light");
	
	checkGLError();
	
}


void Plane::draw(float elapsed, const glm::mat4& view, const glm::mat4& projection, size_t pingpong){
	
	glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,-0.35f,-0.5f)), glm::vec3(2.0f));
	// Combine the three matrices.
	glm::mat4 MV = view * model;
	glm::mat4 MVP = projection * MV;
	
	// Compute the normal matrix
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));
	
	// Select the program (and shaders).
	glUseProgram(_programId);

	// Select the right sub-uniform buffer to use for the light.
	glUniformBlockBinding(_programId, _lightUniformId, pingpong);
	
	// Upload the MVP matrix.
	GLuint mvpID  = glGetUniformLocation(_programId, "mvp");
	glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
	// Upload the MV matrix.
	GLuint mvID  = glGetUniformLocation(_programId, "mv");
	glUniformMatrix4fv(mvID, 1, GL_FALSE, &MV[0][0]);
	// Upload the normal matrix.
	GLuint normalMatrixID  = glGetUniformLocation(_programId, "normalMatrix");
	glUniformMatrix3fv(normalMatrixID, 1, GL_FALSE, &normalMatrix[0][0]);

	
	// Select the geometry.
	glBindVertexArray(_vao);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glDrawElements(GL_TRIANGLES, _count, GL_UNSIGNED_INT, (void*)0);

	glBindVertexArray(0);
	glUseProgram(0);
}

void Plane::drawDepth(float elapsed, const glm::mat4& view, const glm::mat4& projection){
	
	glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,-0.35f,-0.5f)), glm::vec3(2.0f));
	// Combine the three matrices.
	glm::mat4 MV = view * model;
	glm::mat4 MVP = projection * MV;
	
	glUseProgram(_programDepthId);
	
	// Upload the MVP matrix.
	GLuint mvpID  = glGetUniformLocation(_programDepthId, "mvp");
	glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
	
	// Select the geometry.
	glBindVertexArray(_vao);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glDrawElements(GL_TRIANGLES, _count, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(0);
	glUseProgram(0);
}



void Plane::clean(){
	glDeleteVertexArrays(1, &_vao);
}


