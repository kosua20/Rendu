#include <stdio.h>
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

#include "DirectionalLight.h"



DirectionalLight::DirectionalLight(const glm::vec3& worldPosition, const glm::vec3& color, const glm::mat4& projection) : Light(worldPosition, color, projection) {
}


void DirectionalLight::init(std::map<std::string, GLuint> textureIds){
	_screenquad.init(textureIds, "ressources/shaders/directional_light");
}

void DirectionalLight::draw(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix){
	
	glm::mat4 invView = glm::inverse(viewMatrix);
	
	// Store the four variable coefficients of the projection matrix.
	glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	glm::vec3 lightPositionViewSpace = glm::vec3(viewMatrix * glm::vec4(_local, 0.0));
	
	glUseProgram(_screenquad.program());
	
	GLuint lightPosId = glGetUniformLocation(_screenquad.program(), "lightDirection");
	glUniform3fv(lightPosId, 1,  &lightPositionViewSpace[0]);
	
	GLuint lightColId = glGetUniformLocation(_screenquad.program(), "lightColor");
	glUniform3fv(lightColId, 1,  &_color[0]);
	
	// Projection parameter for position reconstruction.
	GLuint projId = glGetUniformLocation(_screenquad.program(), "projectionMatrix");
	glUniform4fv(projId, 1, &(projectionVector[0]));
	
	GLuint invVID  = glGetUniformLocation(_screenquad.program(), "inverseV");
	glUniformMatrix4fv(invVID, 1, GL_FALSE, &invView[0][0]);
	
	_screenquad.draw(invScreenSize);

}

