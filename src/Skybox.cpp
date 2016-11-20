#include <stdio.h>
#include <iostream>
#include <vector>
#include <lodepng/lodepng.h>
#include <glm/gtc/matrix_transform.hpp>

#include "helpers/ProgramUtilities.h"

#include "Skybox.h"

Skybox::Skybox(){}

Skybox::~Skybox(){}

void Skybox::init(){
	
	// Load the shaders
	_programId = createGLProgram("ressources/shaders/skybox_gbuffer.vert","ressources/shaders/skybox_gbuffer.frag");

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
	std::vector<unsigned int> cubeIndices{2, 1, 0, 3, 1, 2, // Front face
		3, 5, 1, 7, 5, 3, // Right face
		7, 4, 5, 6, 4, 7, // Back face
		6, 0, 4, 2, 0, 6, // Left face
		1, 4, 0, 5, 4, 1, // Bottom face
		6, 3, 2, 7, 3, 6  // Top face
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
	
	_texCubeMap = loadTextureCubeMap("ressources/cubemap/cubemap", _programId, 0, "textureCubeMap", true);
	
	checkGLError();
	
}


void Skybox::draw(float elapsed, const glm::mat4& view, const glm::mat4& projection){
	
	glm::mat4 model = glm::scale(glm::mat4(1.0f),glm::vec3(5.0f));
	// Combine the three matrices.
	glm::mat4 MV = view * model;
	// Prevent the skybox from translating.
	MV[3][0] = MV[3][1] = MV[3][2] = 0.0;
	
	glm::mat4 MVP = projection * MV;
	
	// Select the program (and shaders).
	glUseProgram(_programId);

	// Upload the MVP matrix.
	GLuint mvpID  = glGetUniformLocation(_programId, "mvp");
	glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _texCubeMap);
	
	// Select the geometry.
	glBindVertexArray(_vao);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glDrawElements(GL_TRIANGLES, _count, GL_UNSIGNED_INT, (void*)0);

	glBindVertexArray(0);
	glUseProgram(0);
}


void Skybox::clean(){
	glDeleteVertexArrays(1, &_vao);
	glDeleteTextures(1, &_texCubeMap);
}


