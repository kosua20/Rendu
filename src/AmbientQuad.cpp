#include <stdio.h>
#include <iostream>
#include <vector>

#include "helpers/ProgramUtilities.h"

#include "AmbientQuad.h"

AmbientQuad::AmbientQuad(){}

AmbientQuad::~AmbientQuad(){}

void AmbientQuad::init(std::map<std::string, GLuint> textureIds, const std::string & shaderRoot){
	
	ScreenQuad::init(textureIds, shaderRoot);
	
	_texCubeMapSmall = loadTextureCubeMap("ressources/cubemap/cubemap_diff",
										  _programId, (GLuint)_textureIds.size(),
										  "textureCubeMapSmall", true);
	
}

void AmbientQuad::draw(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
	
	glm::mat4 invView = glm::inverse(viewMatrix);
	
	glUseProgram(_programId);
	
	GLuint invVID  = glGetUniformLocation(_programId, "inverseV");
	glUniformMatrix4fv(invVID, 1, GL_FALSE, &invView[0][0]);
	
	
	glActiveTexture(GL_TEXTURE0 + (unsigned int)_textureIds.size());
	glBindTexture(GL_TEXTURE_CUBE_MAP, _texCubeMapSmall);
	
	ScreenQuad::draw(invScreenSize);
}


void AmbientQuad::clean(){
	ScreenQuad::clean();
}
