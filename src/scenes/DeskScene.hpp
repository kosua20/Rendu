#ifndef DeskScene_h
#define DeskScene_h

#include "Scene.hpp"

#include <stdio.h>
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>


class DeskScene : public Scene {
public:
	void init();
	void update(double fullTime, double frameTime);
};


void DeskScene::init(){
	
	// Create directional light.
	//directionalLights.emplace_back(glm::vec3(-2.0f, 1.5f, 0.0f), 4.0f*glm::vec3(1.0f,1.0f, 0.92f), glm::ortho(-2.25f,0.25f,-1.5f,0.75f,1.0f,6.0f));
	
	glm::mat4 sceneMatrix = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)), glm::vec3(0.0f,0.0f,-2.0f));
	// Objects creation.
	Object candle(Object::Type::Regular, "candle", { {"candle_albedo", true }, {"candle_normal", false}, {"candle_rough_met_ao", false}});
	Object desk(Object::Type::Regular, "desk", { {"desk_albedo", true }, {"desk_normal", false}, {"desk_rough_met_ao", false}}, {});
	Object hammer(Object::Type::Regular, "hammer", { {"hammer_albedo", true }, {"hammer_normal", false}, {"hammer_rough_met_ao", false}});
	Object lighter(Object::Type::Regular, "lighter", { {"lighter_albedo", true }, {"lighter_normal", false}, {"lighter_rough_met_ao", false}});
	Object rock(Object::Type::Regular, "rock", { {"rock_albedo", true }, {"rock_normal", false}, {"rock_rough_met_ao", false}});
	Object screwdriver(Object::Type::Regular, "screwdriver", { {"screwdriver_albedo", true }, {"screwdriver_normal", false}, {"screwdriver_rough_met_ao", false}});
	Object spyglass(Object::Type::Regular, "spyglass", { {"spyglass_albedo", true }, {"spyglass_normal", false}, {"spyglass_rough_met_ao", false}});
	objects.push_back(candle);
	objects.push_back(desk);
	objects.push_back(hammer);
	objects.push_back(lighter);
	objects.push_back(rock);
	objects.push_back(screwdriver);
	objects.push_back(spyglass);
	for(auto & object : objects){
		object.update(sceneMatrix);
	}
	
	// Background creation.
	background = Object(Object::Type::Skybox, "skybox", {}, {{"small_apartment", true }});
	backgroundReflection = Resources::manager().getCubemap("small_apartment").id;
	backgroundIrradiance = Resources::manager().getCubemap("small_apartment_irr").id;
}

void DeskScene::update(double fullTime, double frameTime){
	
}

#endif
