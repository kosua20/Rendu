#include <stdio.h>
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

#include "Light.h"


Light::Light() : _lightStruct() { }



Light::Light(const glm::vec4& local, const glm::vec4& Ia, const glm::vec4& Id, const glm::vec4& Is, const float shininess, const glm::mat4 projectionMatrix){
	_local = glm::vec3(local);
	_type = (local[3] == 0.0 ? LightType::Directional : LightType::Point);
	_projectionMatrix = projectionMatrix;
	_viewMatrix = glm::lookAt(_local, glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f));
	_mvp = _projectionMatrix * _viewMatrix;
	_lightStruct = LightInternal(glm::vec4(0.0), Ia, Id, Is, shininess);
	
}

Light::~Light() { }

void Light::update(float time, const glm::mat4& camViewMatrix){
	// Compute the light position/direction in world space.
	_local = glm::vec3(2.0f,(1.5f + sin(0.5*time)),2.0f);
	// Update matrices.
	_viewMatrix = glm::lookAt(_local, glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f));
	_mvp = _projectionMatrix * _viewMatrix;
	// Update camera space position/direction
	_lightStruct._p = camViewMatrix * glm::vec4(_local, _type == LightType::Point ? 1.0f : 0.0f);
	
}





