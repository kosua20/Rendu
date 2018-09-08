#include "AmbientQuad.hpp"
#include "../../resources/ResourcesManager.hpp"
#include "../../helpers/GenerationUtilities.hpp"


AmbientQuad::AmbientQuad(){}

void AmbientQuad::init(std::vector<GLuint> textureIds){
	
	_program = Resources::manager().getProgram2D("ambient");
	// Load texture.
	_textureBrdf = Resources::manager().getTexture("brdf-precomputed", false).id;
	
	// Ambient pass: needs the albedo, the normals, the depth, the effects and the AO result
	_textures = textureIds;
	
	// Setup SSAO data, get back noise texture id, add it to the gbuffer outputs.
	GLuint noiseTextureID = setupSSAO();
	_programSSAO = Resources::manager().getProgram2D("ssao");
	_texturesSSAO = { _textures[2], _textures[1], noiseTextureID };
	// Now that we have the program we can send the samples to the GPU too.
	_programSSAO->cacheUniformArray("samples", _samples);
	
	checkGLError();
}

void AmbientQuad::setSceneParameters(const GLuint reflectionMap, const std::vector<glm::vec3> & irradiance){
	_textureEnv = reflectionMap;
	_program->cacheUniformArray("shCoeffs", irradiance);
}

GLuint AmbientQuad::setupSSAO(){
	// Samples.
	_samples.clear();
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
	// Cubemaps.
	glActiveTexture(GL_TEXTURE0 + (unsigned int)_textures.size());
	glBindTexture(GL_TEXTURE_CUBE_MAP, _textureEnv);
	glActiveTexture(GL_TEXTURE0 + (unsigned int)_textures.size() + 1);
	glBindTexture(GL_TEXTURE_2D, _textureBrdf);
	
	ScreenQuad::draw(_textures);
	checkGLError();
}

void AmbientQuad::drawSSAO(const glm::mat4& projectionMatrix) const {
	
	glUseProgram(_programSSAO->id());
	
	glUniformMatrix4fv(_programSSAO->uniform("projectionMatrix"), 1, GL_FALSE, &projectionMatrix[0][0]);
	
	ScreenQuad::draw(_texturesSSAO);
	
}



void AmbientQuad::clean() const {
	
}
