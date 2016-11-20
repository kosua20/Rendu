#include <stdio.h>
#include <iostream>
#include <vector>

#include "helpers/ProgramUtilities.h"

#include "GbufferQuad.h"

GbufferQuad::GbufferQuad(){}

GbufferQuad::~GbufferQuad(){}

void GbufferQuad::init(std::map<std::string, GLuint> textureIds, const std::string & shaderRoot, GLuint shadowMapTextureId){
	
	ScreenQuad::init(textureIds, shaderRoot);
	
	_texCubeMap = loadTextureCubeMap("ressources/cubemap/cubemap", _programId, _textureIds.size(), "textureCubeMap", true);
	_texCubeMapSmall = loadTextureCubeMap("ressources/cubemap/cubemap_diff", _programId, _textureIds.size()+1, "textureCubeMapSmall", true);
	
	_shadowMapId = shadowMapTextureId;
	glBindTexture(GL_TEXTURE_2D, _shadowMapId);
	GLuint texUniID = glGetUniformLocation(_programId, "shadowMap");
	glUniform1i(texUniID, _textureIds.size()+2);
	
	_lightUniformId = glGetUniformBlockIndex(_programId, "Light");
}

void GbufferQuad::draw(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::mat4& lightMatrix, size_t pingpong) {
	
	glm::mat4 invView = glm::inverse(viewMatrix);
	
	glUseProgram(_programId);
	
	glUniformBlockBinding(_programId, _lightUniformId, pingpong);
	
	// Projection parameter for position reconstruction.
	
	GLuint invVID  = glGetUniformLocation(_programId, "inverseV");
	glUniformMatrix4fv(invVID, 1, GL_FALSE, &invView[0][0]);
	
	GLuint lightVPID  = glGetUniformLocation(_programId, "lightVP");
	glUniformMatrix4fv(lightVPID, 1, GL_FALSE, &lightMatrix[0][0]);
	
	glActiveTexture(GL_TEXTURE0 + _textureIds.size());
	glBindTexture(GL_TEXTURE_CUBE_MAP, _texCubeMap);
	glActiveTexture(GL_TEXTURE0 + _textureIds.size()+1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _texCubeMapSmall);
	glActiveTexture(GL_TEXTURE0 + _textureIds.size()+2);
	glBindTexture(GL_TEXTURE_2D, _shadowMapId);
	
	ScreenQuad::draw(invScreenSize);
}
