#include <stdio.h>
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>


#include "Object.h"

Object::Object(){}

Object::~Object(){}

void Object::init(const std::string& meshPath, const std::vector<std::string>& texturesPaths, int materialId){
	
	// Load the shaders
	_programDepthId = ProgramUtilities::createGLProgram("resources/shaders/lights/object_depth.vert","resources/shaders/lights/object_depth.frag");
	_programId = ProgramUtilities::createGLProgram("resources/shaders/gbuffer/object_gbuffer.vert","resources/shaders/gbuffer/object_gbuffer.frag");
	
	// Load geometry.
	_mesh = Resources::manager().getMesh(meshPath);
	
	// Load and upload the textures.
	_texColor = Resources::manager().getTexture(texturesPaths[0]).id;
	_texNormal = Resources::manager().getTexture(texturesPaths[1], false).id;
	_texEffects = Resources::manager().getTexture(texturesPaths[2], false).id;
	
	glUseProgram(_programId);
	glUniform1i(glGetUniformLocation(_programId, "textureColor"), 0);
	glUniform1i(glGetUniformLocation(_programId, "textureNormal"), 1);
	glUniform1i(glGetUniformLocation(_programId, "textureEffects"), 2);
	
	_mvpId  = glGetUniformLocation(_programId, "mvp");
	_mvId  = glGetUniformLocation(_programId, "mv");
	_normalMatrixId  = glGetUniformLocation(_programId, "normalMatrix");
	_pId  = glGetUniformLocation(_programId, "p");
	// This one won't be modified, no need to keep the ID around.
	GLuint matIdID  = glGetUniformLocation(_programId, "materialId");
	glUniform1i(matIdID, materialId);
	
	glUseProgram(_programDepthId);
	_mvpDepthId  = glGetUniformLocation(_programDepthId, "mvp");
	glUseProgram(0);

	
	checkGLError();
	
}

void Object::update(const glm::mat4& model){
	
	_model = model;
	
}


void Object::draw(const glm::mat4& view, const glm::mat4& projection) const {
	
	// Combine the three matrices.
	glm::mat4 MV = view * _model;
	glm::mat4 MVP = projection * MV;

	// Compute the normal matrix
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(MV)));
	// Select the program (and shaders).
	glUseProgram(_programId);
	
	// Upload the MVP matrix.
	glUniformMatrix4fv(_mvpId, 1, GL_FALSE, &MVP[0][0]);
	// Upload the MV matrix.
	glUniformMatrix4fv(_mvId, 1, GL_FALSE, &MV[0][0]);
	// Upload the normal matrix.
	glUniformMatrix3fv(_normalMatrixId, 1, GL_FALSE, &normalMatrix[0][0]);
	// Upload the projection matrix.
	glUniformMatrix4fv(_pId, 1, GL_FALSE, &projection[0][0]);

	// Bind the textures.
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texColor);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _texNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _texEffects);
	
	// Select the geometry.
	glBindVertexArray(_mesh.vId);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _mesh.eId);
	glDrawElements(GL_TRIANGLES, _mesh.count, GL_UNSIGNED_INT, (void*)0);

	glBindVertexArray(0);
	glUseProgram(0);
	
	
}


void Object::drawDepth(const glm::mat4& lightVP) const {
	
	// Combine the three matrices.
	glm::mat4 lightMVP = lightVP * _model;
	
	glUseProgram(_programDepthId);
	
	// Upload the MVP matrix.
	glUniformMatrix4fv(_mvpDepthId, 1, GL_FALSE, &lightMVP[0][0]);
	
	// Select the geometry.
	glBindVertexArray(_mesh.vId);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _mesh.eId);
	glDrawElements(GL_TRIANGLES, _mesh.count, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(0);
	glUseProgram(0);
	
}


void Object::clean() const {
	glDeleteVertexArrays(1, &_mesh.vId);
	glDeleteTextures(1, &_texColor);
	glDeleteTextures(1, &_texNormal);
	glDeleteTextures(1, &_texEffects);
	glDeleteProgram(_programId);
}


