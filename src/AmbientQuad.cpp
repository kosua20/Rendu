#include "AmbientQuad.hpp"
#include "helpers/ResourcesManager.hpp"
#include "helpers/GenerationUtilities.hpp"

#include <stdio.h>
#include <iostream>
#include <vector>



AmbientQuad::AmbientQuad(){}

AmbientQuad::~AmbientQuad(){}

void AmbientQuad::init(std::map<std::string, GLuint> textureIds, const GLuint reflection, const GLuint irradiance){
	
	// Ambient pass: needs the albedo, the normals, the effect and the AO result
	std::map<std::string, GLuint> finalTextures = { {"albedoTexture", textureIds["albedoTexture"]}, {"normalTexture", textureIds["normalTexture"]}, {"depthTexture", textureIds["depthTexture"]},  {"effectsTexture", textureIds["effectsTexture"]}, {"ssaoTexture", textureIds["ssaoTexture"]}};
	
	ScreenQuad::init(finalTextures, "ambient");
	
	// Load texture.
	_texCubeMap = reflection;
	_texCubeMapSmall = irradiance;
	// Bind uniform to texture slot.
	_program->registerTexture("textureCubeMap", (int)_textureIds.size());
	_program->registerTexture("textureCubeMapSmall", (int)_textureIds.size()+1);
	
	// Setup SSAO data, get back noise texture id, add it to the gbuffer outputs.
	GLuint noiseTextureID = setupSSAO();
	std::map<std::string, GLuint> ssaoTextures = { {"depthTexture", textureIds["depthTexture"]}, {"normalTexture", textureIds["normalTexture"]}, {"noiseTexture",noiseTextureID}};
	_ssaoScreen.init(ssaoTextures, "ssao");
	
	// Now that we have the program we can send the samples to the GPU too.
	_ssaoScreen.program()->cacheUniformArray("samples", _samples);
	
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
		float scale = i/24.0f;
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

void AmbientQuad::draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	
	glm::mat4 invView = glm::inverse(viewMatrix);
	// Store the four variable coefficients of the projection matrix.
	glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	
	glUseProgram(_program->id());
	
	glUniformMatrix4fv(_program->uniform("inverseV"), 1, GL_FALSE, &invView[0][0]);
	glUniform4fv(_program->uniform("projectionMatrix"), 1, &(projectionVector[0]));
	
	glActiveTexture(GL_TEXTURE0 + (unsigned int)_textureIds.size());
	glBindTexture(GL_TEXTURE_CUBE_MAP, _texCubeMap);
	
	glActiveTexture(GL_TEXTURE0 + (unsigned int)_textureIds.size() + 1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _texCubeMapSmall);
	
	ScreenQuad::draw();
}

void AmbientQuad::drawSSAO(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	
	glUseProgram(_ssaoScreen.program()->id());
	
	glUniformMatrix4fv(_ssaoScreen.program()->uniform("projectionMatrix"), 1, GL_FALSE, &projectionMatrix[0][0]);
	
	_ssaoScreen.draw();
	
}



void AmbientQuad::clean() const {
	ScreenQuad::clean();
	_ssaoScreen.clean();
}
