#include <stdio.h>
#include <iostream>
#include <vector>

#include "helpers/ProgramUtilities.h"

#include "GbufferQuad.h"

GbufferQuad::GbufferQuad(){}

GbufferQuad::~GbufferQuad(){}

void GbufferQuad::init(std::map<std::string, GLuint> textureIds, const std::string & shaderRoot, GLuint shadowMapTextureId){
	
	ScreenQuad::init(textureIds, shaderRoot);
	
	_texCubeMap = loadTextureCubeMap("ressources/cubemap/cubemap",
									 _programId, (GLuint)_textureIds.size(),
									 "textureCubeMap", true);
	_texCubeMapSmall = loadTextureCubeMap("ressources/cubemap/cubemap_diff",
										  _programId, (GLuint)_textureIds.size()+1,
										  "textureCubeMapSmall", true);
	
	_shadowMapId = shadowMapTextureId;
	glBindTexture(GL_TEXTURE_2D, _shadowMapId);
	GLuint texUniID = glGetUniformLocation(_programId, "shadowMap");
	glUniform1i(texUniID, (GLuint)_textureIds.size()+2);
	
	_lightUniformId = glGetUniformBlockIndex(_programId, "Light");
}

void GbufferQuad::draw(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::mat4& lightMatrix, GLuint pingpong) {
	
	glm::mat4 invView = glm::inverse(viewMatrix);
	
	// Store the four variable coefficients of the projection matrix.
	glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	
	glUseProgram(_programId);
	
	glUniformBlockBinding(_programId, _lightUniformId, pingpong);
	
	// Projection parameter for position reconstruction.
	GLuint projId = glGetUniformLocation(_programId, "projectionMatrix");
	glUniform4fv(projId, 1, &(projectionVector[0]));
	
	GLuint invVID  = glGetUniformLocation(_programId, "inverseV");
	glUniformMatrix4fv(invVID, 1, GL_FALSE, &invView[0][0]);
	
	GLuint lightVPID  = glGetUniformLocation(_programId, "lightVP");
	glUniformMatrix4fv(lightVPID, 1, GL_FALSE, &lightMatrix[0][0]);
	
	glActiveTexture(GL_TEXTURE0 + (unsigned int)_textureIds.size());
	glBindTexture(GL_TEXTURE_CUBE_MAP, _texCubeMap);
	glActiveTexture(GL_TEXTURE0 + (unsigned int)_textureIds.size() + 1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _texCubeMapSmall);
	glActiveTexture(GL_TEXTURE0 + (unsigned int)_textureIds.size() + 2);
	glBindTexture(GL_TEXTURE_2D, _shadowMapId);
	
	ScreenQuad::draw(invScreenSize);
}
