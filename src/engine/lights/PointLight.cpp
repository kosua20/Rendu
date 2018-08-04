#include "PointLight.hpp"

#include <stdio.h>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>





PointLight::PointLight(const glm::vec3& worldPosition, const glm::vec3& color, float radius) : Light(color) {
	_radius = radius;
	_lightPosition = worldPosition;
}


void PointLight::loadProgramAndGeometry() {
	_debugProgram = Resources::manager().getProgram("point_light_debug", "point_light", "light_debug");
	_debugMesh = Resources::manager().getMesh("light_sphere");
	checkGLError();
}

void PointLight::init(const std::map<std::string, GLuint>& textureIds){
	_program = Resources::manager().getProgram("point_light");
	//glUseProgram(_program->id());
	checkGLError();
	GLint currentTextureSlot = 0;
	_textureIds.clear();
	for(auto& texture : textureIds){
		_textureIds.push_back(texture.second);
		//glBindTexture(GL_TEXTURE_2D, _textureIds.back());
		_program->registerTexture(texture.first, currentTextureSlot);
		currentTextureSlot += 1;
	}
	
	checkGLError();
}


void PointLight::draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize ) const {
	
	// Store the four variable coefficients of the projection matrix.
	glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	glm::vec3 lightPositionViewSpace = glm::vec3(viewMatrix * glm::vec4(_lightPosition, 1.0f));
	glm::mat4 vp = projectionMatrix * viewMatrix;
	
	glUseProgram(_program->id());
	
	// For the vertex shader
	glUniform1f(_program->uniform("radius"),  _radius);
	glUniform3fv(_program->uniform("lightWorldPosition"), 1, &_lightPosition[0]);
	glUniformMatrix4fv(_program->uniform("mvp"), 1, GL_FALSE, &vp[0][0]);
	glUniform3fv(_program->uniform("lightPosition"), 1,  &lightPositionViewSpace[0]);
	glUniform3fv(_program->uniform("lightColor"), 1,  &_color[0]);
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
	glBindVertexArray(_debugMesh.vId);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _debugMesh.eId);
	glDrawElements(GL_TRIANGLES, _debugMesh.count, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(0);
	glUseProgram(0);
	
}

void PointLight::drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	glm::mat4 vp = projectionMatrix * viewMatrix;
	
	glUseProgram(_debugProgram->id());
	
	// For the vertex shader
	glUniform1f(_debugProgram->uniform("radius"),  0.05f*_radius);
	glUniform3fv(_debugProgram->uniform("lightWorldPosition"), 1, &_lightPosition[0]);
	glUniformMatrix4fv(_debugProgram->uniform("mvp"), 1, GL_FALSE, &vp[0][0]);
	glUniform3fv(_debugProgram->uniform("lightColor"), 1,  &_color[0]);
	
	// Select the geometry.
	glBindVertexArray(_debugMesh.vId);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _debugMesh.eId);
	glDrawElements(GL_TRIANGLES, _debugMesh.count, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(0);
	glUseProgram(0);
}


void PointLight::update(const glm::vec3 & newPosition){
	_lightPosition = newPosition;
}

void PointLight::clean() const {
	
}

std::shared_ptr<ProgramInfos> PointLight::_debugProgram;
MeshInfos PointLight::_debugMesh;




