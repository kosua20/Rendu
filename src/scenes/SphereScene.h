#ifndef SphereScene_h
#define SphereScene_h

#include "Scene.h"

#include <stdio.h>
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>


class SphereScene : public Scene {
public:
	void init();
	void update(double timer, double elapsedTime);
};


void SphereScene::init(){
	
	// Create directional light.
	directionalLights.emplace_back(glm::vec3(-2.0f, 1.5f, 0.0f), 5.0f*glm::vec3(1.0f,1.0f, 0.92f), glm::ortho(-0.75f,0.75f,-0.75f,0.75f,1.0f,6.0f));
	
	pointLights.emplace_back( glm::vec3(0.5f,-0.1f,0.5f), 3.0f*glm::vec3(0.2f,0.8f,1.2f), 0.9f);
	
	// Objects creation.
	Object sphere(Object::Type::Regular, "sphere", { {"sphere_wood_lacquered_albedo", true }, {"sphere_wood_lacquered_normal", false}, {"sphere_wood_lacquered_metallic", false}});
	const glm::mat4 model = glm::scale(glm::mat4(1.0f),glm::vec3(0.35f));
	sphere.update(model);
	objects.push_back(sphere);
	
	// Background creation.
	background = Object(Object::Type::Skybox, "skybox", {}, {{"studio", true }});
	
}

void SphereScene::update(double timer, double elapsedTime){
	const glm::mat4 model = glm::rotate(glm::scale(glm::mat4(1.0f),glm::vec3(0.35f)), 0.2f*float(timer), glm::vec3(0.0f,1.0f,0.0f));
	objects[0].update(model);
	
}

#endif
