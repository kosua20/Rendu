#include <stdio.h>
#include <iostream>
#include <vector>

#include "helpers/ProgramUtilities.h"
#include "helpers/GenerationUtilities.h"

#include "AmbientQuad.h"

AmbientQuad::AmbientQuad(){}

AmbientQuad::~AmbientQuad(){}

void AmbientQuad::init(std::map<std::string, GLuint> textureIds){
	
	// Ambient pass: needs the albedo, the normals and the AO result
	std::map<std::string, GLuint> finalTextures = { {"albedoTexture", textureIds["albedoTexture"]}, {"normalTexture", textureIds["normalTexture"]}, {"ssaoTexture", textureIds["ssaoTexture"]}};
	ScreenQuad::init(finalTextures, "ressources/shaders/gbuffer/ambient");
	
	// Load texture.
	_texCubeMapSmall = loadTextureCubeMap("ressources/cubemap/cubemap_diff", true);
	// Bind uniform to texture slot.
	glUseProgram(_programId);
	glUniform1i(glGetUniformLocation(_programId, "textureCubeMapSmall"), (GLuint)_textureIds.size());
	glUseProgram(0);
	
	// Setup SSAO data, get back noise texture id, add it to the gbuffer outputs.
	GLuint noiseTextureID = setupSSAO();
	std::map<std::string, GLuint> ssaoTextures = { {"depthTexture", textureIds["depthTexture"]}, {"normalTexture", textureIds["normalTexture"]}, {"noiseTexture",noiseTextureID}};
	_ssaoScreen.init(ssaoTextures, "ressources/shaders/gbuffer/ssao");
	
	// Now that we have the program we can send the samples to the GPU too.
	glUseProgram(_ssaoScreen.program());
	for(int i = 0; i < 24; ++i){
		const std::string name = "samples[" + std::to_string(i) + "]";
		const GLuint location = glGetUniformLocation(_ssaoScreen.program(), name.c_str());
		glUniform3fv(location, 1, &_samples[i][0] );
	}
	
	glUseProgram(_programId);
	_invVId  = glGetUniformLocation(_programId, "inverseV");
	glUseProgram(_ssaoScreen.program());
	_invPId  = glGetUniformLocation(_ssaoScreen.program(), "projectionMatrix");
	glUseProgram(0);
	checkGLError();
}

GLuint AmbientQuad::setupSSAO(){
	// Samples.
	// We need random vectors in the half sphere above z, with more samples close to the center.
	for(int i = 0; i < 24; ++i){
		glm::vec3 randVec = glm::vec3(Random::Float(-1.0f, 1.0f),
									  Random::Float(-1.0f, 1.0f),
									  Random::Float(0.0f, 1.0f) );
		_samples.push_back(glm::normalize(randVec));
		_samples.back() *= Random::Float(0.0f,1.0f);
		// Skew the distribution towards the center.
		float scale = i/24.0;
		scale = 0.1f+0.9f*scale*scale;
		_samples.back() *= scale;
	}
	
	// Noise texture (same size as the box blur applied after SSAO computation).
	// We need to generate two dimensional normalized offsets.
	std::vector<glm::vec3> noise;
	for(int i = 0; i < 25; ++i){
		glm::vec3 randVec = glm::vec3(Random::Float(-1.0f, 1.0f),
									  Random::Float(-1.0f, 1.0f),
									  0.0f);
		noise.push_back(glm::normalize(randVec));
	}
	
	// Send the texture to the GPU.
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 5 , 5, 0, GL_RGB, GL_FLOAT, &(noise[0]));
	// Need nearest filtering and repeat.
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_REPEAT);
	checkGLError();
	return textureId;
}

void AmbientQuad::draw(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	
	glm::mat4 invView = glm::inverse(viewMatrix);
	
	glUseProgram(_programId);
	
	glUniformMatrix4fv(_invVId, 1, GL_FALSE, &invView[0][0]);
	
	glActiveTexture(GL_TEXTURE0 + (unsigned int)_textureIds.size());
	glBindTexture(GL_TEXTURE_CUBE_MAP, _texCubeMapSmall);
	
	ScreenQuad::draw(invScreenSize);
}

void AmbientQuad::drawSSAO(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	
	glUseProgram(_ssaoScreen.program());
	
	glUniformMatrix4fv(_invPId, 1, GL_FALSE, &projectionMatrix[0][0]);
	
	_ssaoScreen.draw(invScreenSize);
	
}



void AmbientQuad::clean() const {
	ScreenQuad::clean();
	_ssaoScreen.clean();
}
