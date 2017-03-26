#include <stdio.h>
#include <iostream>
#include <vector>

#include "helpers/ProgramUtilities.h"

#include "AmbientQuad.h"

AmbientQuad::AmbientQuad(){}

AmbientQuad::~AmbientQuad(){}

void AmbientQuad::init(std::map<std::string, GLuint> textureIds, const std::string & shaderRoot){
	
	ScreenQuad::init(textureIds, shaderRoot);
	
	_texCubeMap = loadTextureCubeMap("ressources/cubemap/cubemap",
									 _programId, (GLuint)_textureIds.size(),
									 "textureCubeMap", true);
	_texCubeMapSmall = loadTextureCubeMap("ressources/cubemap/cubemap_diff",
										  _programId, (GLuint)_textureIds.size()+1,
										  "textureCubeMapSmall", true);
	
}

void AmbientQuad::draw(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
	
	glm::mat4 invView = glm::inverse(viewMatrix);
	
	// Store the four variable coefficients of the projection matrix.
	glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	
	glUseProgram(_programId);
	
		// Projection parameter for position reconstruction.
	GLuint projId = glGetUniformLocation(_programId, "projectionMatrix");
	glUniform4fv(projId, 1, &(projectionVector[0]));
	
	GLuint invVID  = glGetUniformLocation(_programId, "inverseV");
	glUniformMatrix4fv(invVID, 1, GL_FALSE, &invView[0][0]);
	
	
	glActiveTexture(GL_TEXTURE0 + (unsigned int)_textureIds.size());
	glBindTexture(GL_TEXTURE_CUBE_MAP, _texCubeMap);
	glActiveTexture(GL_TEXTURE0 + (unsigned int)_textureIds.size() + 1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _texCubeMapSmall);
	
	ScreenQuad::draw(invScreenSize);
}
