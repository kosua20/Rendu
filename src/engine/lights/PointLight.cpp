#include "PointLight.hpp"

#include <stdio.h>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>


PointLight::PointLight(const glm::vec3& worldPosition, const glm::vec3& color, float radius) : Light(color) {
	_radius = radius;
	_lightPosition = worldPosition;
}

void PointLight::init(const std::map<std::string, GLuint>& textureIds){
	_program = Resources::manager().getProgram("point_light", "object_basic", "point_light");
	_sphere = Resources::manager().getMesh("light_sphere");
	GLint currentTextureSlot = 0;
	_textureIds.clear();
	for(auto& texture : textureIds){
		_textureIds.push_back(texture.second);
		_program->registerTexture(texture.first, currentTextureSlot);
		currentTextureSlot += 1;
	}
	checkGLError();
}


void PointLight::draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize ) const {
	
	// Store the four variable coefficients of the projection matrix.
	const glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	const glm::vec3 lightPositionViewSpace = glm::vec3(viewMatrix * glm::vec4(_lightPosition, 1.0f));
	// Compute the model matrix to scale the sphere based on the radius.
	const glm::mat4 modelMatrix = glm::scale(glm::translate(glm::mat4(1.0f), _lightPosition), glm::vec3(_radius));
	const glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
	
	glUseProgram(_program->id());
	glUniformMatrix4fv(_program->uniform("mvp"), 1, GL_FALSE, &mvp[0][0]);
	glUniform3fv(_program->uniform("lightPosition"), 1,  &lightPositionViewSpace[0]);
	glUniform3fv(_program->uniform("lightColor"), 1,  &_color[0]);
	glUniform1f(_program->uniform("lightRadius"), _radius);
	// Projection parameter for position reconstruction.
	glUniform4fv(_program->uniform("projectionMatrix"), 1, &(projectionVector[0]));
	// Inverse screen size uniform.
	glUniform2fv(_program->uniform("inverseScreenSize"), 1, &(invScreenSize[0]));
	
	// Active screen texture.
	for(GLuint i = 0;i < _textureIds.size(); ++i){
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, _textureIds[i]);
	}
	
	// Select the geometry.
	glBindVertexArray(_sphere.vId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _sphere.eId);
	glDrawElements(GL_TRIANGLES, _sphere.count, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(0);
	glUseProgram(0);
}

void PointLight::drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	
	const std::shared_ptr<ProgramInfos> debugProgram = Resources::manager().getProgram("light_debug", "object_basic", "light_debug");
	
	// Compute the model matrix to scale the sphere based on the radius.
	const glm::mat4 modelMatrix = glm::scale(glm::translate(glm::mat4(1.0f), _lightPosition), glm::vec3(_radius));
	const glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
	const glm::vec3 colorLow = _color/(std::max)(_color[0], (std::max)(_color[1], _color[2]));
	
	glUseProgram(debugProgram->id());
	glUniformMatrix4fv(debugProgram->uniform("mvp"), 1, GL_FALSE, &mvp[0][0]);
	glUniform3fv(debugProgram->uniform("lightColor"), 1,  &colorLow[0]);
	
	glBindVertexArray(_sphere.vId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _sphere.eId);
	glDrawElements(GL_TRIANGLES, _sphere.count, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);
	glUseProgram(0);

}


void PointLight::update(const glm::vec3 & newPosition){
	_lightPosition = newPosition;
}

void PointLight::clean() const {
	
}
