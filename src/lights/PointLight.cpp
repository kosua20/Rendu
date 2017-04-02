#include <stdio.h>
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include "../helpers/ProgramUtilities.h"
#include "../helpers/MeshUtilities.h"

#include "PointLight.h"


PointLight::PointLight(const glm::vec3& worldPosition, const glm::vec3& color, float radius, const glm::mat4& projection ) : Light(worldPosition, color, projection) {
	_radius = radius;
}


void PointLight::loadProgramAndGeometry(){
	
	_debugProgramId = createGLProgram("ressources/shaders/lights/point_light_debug.vert", "ressources/shaders/lights/point_light_debug.frag");
	
	// Load geometry.
	mesh_t mesh;
	loadObj("ressources/sphere.obj",mesh,Indexed);
	
	_count = (GLsizei)mesh.indices.size();
	
	// Create an array buffer to host the geometry data.
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// Upload the data to the Array buffer.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mesh.positions.size() * 3, &(mesh.positions[0]), GL_STATIC_DRAW);
	
	// Generate a vertex array (useful when we add other attributes to the geometry).
	_vao = 0;
	glGenVertexArrays (1, &_vao);
	glBindVertexArray(_vao);
	// The first attribute will be the vertices positions.
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	
	// We load the indices data
	glGenBuffers(1, &_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh.indices.size(), &(mesh.indices[0]), GL_STATIC_DRAW);
	
	glBindVertexArray(0);
	checkGLError();
}

void PointLight::init(const std::map<std::string, GLuint>& textureIds){
	_programId = createGLProgram("ressources/shaders/lights/point_light.vert", "ressources/shaders/lights/point_light.frag");
	
	checkGLError();
	GLint currentTextureSlot = 0;
	for(auto& texture : textureIds){
		_textureIds.push_back(texture.second);
		glBindTexture(GL_TEXTURE_2D, _textureIds.back());
		GLuint texUniID = glGetUniformLocation(_programId, (texture.first).c_str());
		glUniform1i(texUniID, currentTextureSlot);
		
		currentTextureSlot += 1;
		
	}
	
	checkGLError();
}


void PointLight::draw(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix){
	
	// Store the four variable coefficients of the projection matrix.
	glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	glm::vec3 lightPositionViewSpace = glm::vec3(viewMatrix * glm::vec4(_local, 1.0f));
	
	glm::mat4 vp = projectionMatrix * viewMatrix;
	
	glUseProgram(_programId);
	
	// For the vertex shader
	GLuint radiusId = glGetUniformLocation(_programId, "radius");
	glUniform1f(radiusId,  _radius);
	GLuint positionId = glGetUniformLocation(_programId, "lightWorldPosition");
	glUniform3fv(positionId, 1, &_local[0]);
	
	GLuint MVPID  = glGetUniformLocation(_programId, "mvp");
	glUniformMatrix4fv(MVPID, 1, GL_FALSE, &vp[0][0]);

	
	GLuint lightPosId = glGetUniformLocation(_programId, "lightPosition");
	glUniform3fv(lightPosId, 1,  &lightPositionViewSpace[0]);
	
	GLuint lightColId = glGetUniformLocation(_programId, "lightColor");
	glUniform3fv(lightColId, 1,  &_color[0]);
	
	// Projection parameter for position reconstruction.
	GLuint projId = glGetUniformLocation(_programId, "projectionMatrix");
	glUniform4fv(projId, 1, &(projectionVector[0]));
	
	
	// Inverse screen size uniform.
	GLuint screenId = glGetUniformLocation(_programId, "inverseScreenSize");
	glUniform2fv(screenId, 1, &(invScreenSize[0]));
	
	// Active screen texture.
	for(GLuint i = 0;i < _textureIds.size(); ++i){
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, _textureIds[i]);
	}
	
	
	// Select the geometry.
	glBindVertexArray(_vao);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glDrawElements(GL_TRIANGLES, _count, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(0);
	glUseProgram(0);
	
}

void PointLight::drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix){
	glm::mat4 vp = projectionMatrix * viewMatrix;
	
	glUseProgram(_debugProgramId);
	
	// For the vertex shader
	GLuint radiusId = glGetUniformLocation(_debugProgramId, "radius");
	glUniform1f(radiusId,  0.1*_radius);
	GLuint positionId = glGetUniformLocation(_debugProgramId, "lightWorldPosition");
	glUniform3fv(positionId, 1, &_local[0]);
	
	GLuint MVPID  = glGetUniformLocation(_debugProgramId, "mvp");
	glUniformMatrix4fv(MVPID, 1, GL_FALSE, &vp[0][0]);
	
	GLuint lightColId = glGetUniformLocation(_debugProgramId, "lightColor");
	glUniform3fv(lightColId, 1,  &_color[0]);
	
	// Select the geometry.
	glBindVertexArray(_vao);
	// Draw!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glDrawElements(GL_TRIANGLES, _count, GL_UNSIGNED_INT, (void*)0);
	
	glBindVertexArray(0);
	glUseProgram(0);
}


void PointLight::clean(){
	
}

GLuint PointLight::_debugProgramId;
GLuint PointLight::_ebo;
GLuint PointLight::_vao;
GLsizei PointLight::_count;




