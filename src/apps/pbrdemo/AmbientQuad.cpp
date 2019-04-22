#include "AmbientQuad.hpp"
#include "resources/ResourcesManager.hpp"
#include "helpers/GenerationUtilities.hpp"


AmbientQuad::AmbientQuad(){}

void AmbientQuad::init(const GLuint texAlbedo, const GLuint texNormals, const GLuint texEffects, const GLuint texDepth, const GLuint texSSAO){
	
	_program = Resources::manager().getProgram2D("ambient");
	// Load texture.
	_textureBrdf = Resources::manager().getTexture("brdf-precomputed", {GL_RG32F, GL_LINEAR, GL_CLAMP_TO_EDGE})->id;
	
	// Ambient pass: needs the albedo, the normals, the depth, the effects and the AO result
	_textures = { texAlbedo, texNormals, texEffects, texDepth, texSSAO };
	
	checkGLError();
}

void AmbientQuad::setSceneParameters(const GLuint reflectionMap, const std::vector<glm::vec3> & irradiance){
	_textureEnv = reflectionMap;
	_program->cacheUniformArray("shCoeffs", irradiance);
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

