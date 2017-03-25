#include <stdio.h>
#include <iostream>
#include <vector>
#include <lodepng/lodepng.h>
#include <glm/gtc/matrix_transform.hpp>

#include "helpers/ProgramUtilities.h"
#include "helpers/MeshUtilities.h"

#include "Object.h"

Object::Object(){}

Object::~Object(){}

void Object::init(const std::string& meshPath, const std::vector<std::string>& texturesPaths, int materialId){
	
	// Load the shaders
	_programDepthId = createGLProgram("ressources/shaders/object_depth.vert","ressources/shaders/object_depth.frag");
	_programId = createGLProgram("ressources/shaders/object_gbuffer.vert","ressources/shaders/object_gbuffer.frag");
	
	// Load geometry.
	mesh_t mesh;
	loadObj(meshPath,mesh,Indexed);
	centerAndUnitMesh(mesh);
	computeTangentsAndBinormals(mesh);

	_count = (GLsizei)mesh.indices.size();
	
	// Create an array buffer to host the geometry data.
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// Upload the data to the Array buffer.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.positions.size() * 3, &(mesh.positions[0]), GL_STATIC_DRAW);

	GLuint vbo_nor = 0;
	glGenBuffers(1, &vbo_nor);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_nor);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.normals.size() * 3, &(mesh.normals[0]), GL_STATIC_DRAW);

	GLuint vbo_uv = 0;
	glGenBuffers(1, &vbo_uv);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_uv);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.texcoords.size() * 2, &(mesh.texcoords[0]), GL_STATIC_DRAW);

	GLuint vbo_tan = 0;
	glGenBuffers(1, &vbo_tan);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_tan);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.tangents.size() * 3, &(mesh.tangents[0]), GL_STATIC_DRAW);

	GLuint vbo_binor = 0;
	glGenBuffers(1, &vbo_binor);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_binor);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.binormals.size() * 3, &(mesh.binormals[0]), GL_STATIC_DRAW);

	// Generate a vertex array (useful when we add other attributes to the geometry).
	_vao = 0;
	glGenVertexArrays (1, &_vao);
	glBindVertexArray(_vao);
	// The first attribute will be the vertices positions.
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// The second attribute will be the normals.
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_nor);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// The third attribute will be the uvs.
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_uv);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	// The fourth attribute will be the tangents.
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_tan);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// The fifth attribute will be the binormals.
	glEnableVertexAttribArray(4);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_binor);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	// We load the indices data
	glGenBuffers(1, &_ebo);
 	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
 	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh.indices.size(), &(mesh.indices[0]), GL_STATIC_DRAW);

	glBindVertexArray(0);
	
	// Load and upload the textures.
	_texColor = loadTexture(texturesPaths[0], _programId, 0,  "textureColor", true);
	
	_texNormal = loadTexture(texturesPaths[1], _programId, 1, "textureNormal");
	
	_texEffects = loadTexture(texturesPaths[2], _programId, 2, "textureEffects");
	
	GLuint matIdID  = glGetUniformLocation(_programId, "materialId");
	glUniform1i(matIdID, materialId);
	
	checkGLError();
	
	
}

void Object::update(const glm::mat4& model){
	
	_model = model;
	
}

void Object::draw(const glm::mat4& view, const glm::mat4& projection){
	
	// Combine the three matrices.
	glm::mat4 MV = view * _model;
	glm::mat4 MVP = projection * MV;

	// Compute the normal matrix
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));
	// Select the program (and shaders).
	glUseProgram(_programId);
	
	// Upload the MVP matrix.
	GLuint mvpID  = glGetUniformLocation(_programId, "mvp");
	glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
	// Upload the MV matrix.
	GLuint mvID  = glGetUniformLocation(_programId, "mv");
	glUniformMatrix4fv(mvID, 1, GL_FALSE, &MV[0][0]);
	// Upload the normal matrix.
	GLuint normalMatrixID  = glGetUniformLocation(_programId, "normalMatrix");
	glUniformMatrix3fv(normalMatrixID, 1, GL_FALSE, &normalMatrix[0][0]);
	// Upload the projection matrix.
	GLuint pID  = glGetUniformLocation(_programId, "p");
	glUniformMatrix4fv(pID, 1, GL_FALSE, &projection[0][0]);

	// Bind the textures.
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texColor);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _texNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _texEffects);
	
	// Select the geometry.
	glBindVertexArray(_vao);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glDrawElements(GL_TRIANGLES, _count, GL_UNSIGNED_INT, (void*)0);

	glBindVertexArray(0);
	glUseProgram(0);
	
	
}


void Object::drawDepth(const glm::mat4& lightVP){
	
	// Combine the three matrices.
	glm::mat4 lightMVP = lightVP * _model;
	
	glUseProgram(_programDepthId);
	
	// Upload the MVP matrix.
	GLuint mvpID  = glGetUniformLocation(_programDepthId, "mvp");
	glUniformMatrix4fv(mvpID, 1, GL_FALSE, &lightMVP[0][0]);
	
	// Select the geometry.
	glBindVertexArray(_vao);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glDrawElements(GL_TRIANGLES, _count, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(0);
	glUseProgram(0);
	
	
}


void Object::clean(){
	glDeleteVertexArrays(1, &_vao);
	glDeleteTextures(1, &_texColor);
	glDeleteTextures(1, &_texNormal);
	glDeleteTextures(1, &_texEffects);
	glDeleteProgram(_programId);
}


