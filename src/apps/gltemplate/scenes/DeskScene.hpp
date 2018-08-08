#ifndef DeskScene_h
#define DeskScene_h

#include "Scene.hpp"


class DeskScene : public Scene {
public:
	void init();
	void update(double fullTime, double frameTime);
	
};


void DeskScene::init(){
	if(_loaded){
		return;
	}
	_loaded = true;
	
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
	loadSphericalHarmonics("small_apartment_shcoeffs");
	
	// Compute the bounding box of the shadow casters.
	const BoundingBox bbox = computeBoundingBox(true);
	
	// Lights creation.
	
	// Create directional lights.
	//directionalLights.emplace_back(glm::vec3(-2.0f, -1.5f, -1.0f), glm::vec3(0.5f,0.65f, 1.3f), bbox);
	//directionalLights.emplace_back(glm::vec3(2.0f, -3.5f, -2.0f), glm::vec3(1.2f,0.9f, 0.2f), bbox);
	//directionalLights[0].castShadow(true);
	//directionalLights[1].castShadow(true);
	
	// Create point candle light.
	const glm::vec3 candleLightPosition = glm::vec3(0.09f,0.52f,-0.36f);
	pointLights.emplace_back(candleLightPosition, glm::vec3(3.0f, 2.0f, 0.2f), 2.5f, bbox);
	pointLights[0].castShadow(true);
}

void DeskScene::update(double fullTime, double frameTime){
	// Quick tentative of mimicking a flickering light.
	/*_frameCount = (_frameCount+Random::Int(1,2))%10;
	if(_frameCount == 0){
		const float delta = 0.0006f;
		const glm::vec3 randomShift(Random::Float(-delta, delta), 0.0f, Random::Float(-delta, delta));
		pointLights[0].update(_candleLightPosition + randomShift);
		pointLights[0].setIntensity(Random::Float(0.8f,1.1f)*glm::vec3(3.0f, 2.0f, 0.2f));
	}
	 */
	
}

#endif
