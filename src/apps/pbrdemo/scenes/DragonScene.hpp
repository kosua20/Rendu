#ifndef DragonScene_h
#define DragonScene_h

#include "Scene.hpp"


class DragonScene : public Scene {
public:
	void init();
	void update(double fullTime, double frameTime);
};


void DragonScene::init(){
	if(_loaded){
		return;
	}
	_loaded = true;
	
	//Position fixed objects.
	const glm::mat4 dragonModel = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-0.1,-0.05,-0.25)),glm::vec3(0.5f));
	const glm::mat4 planeModel = glm::scale(glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,-0.35f,-0.5f)), glm::vec3(2.0f));
	const glm::mat4 suzanneModel = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.2,0.0,0.0)),glm::vec3(0.25f));
	
	// Objects creation.
	Object suzanne(Object::Type::Regular, "suzanne", { {"suzanne_texture_color", true }, {"suzanne_texture_normal", false}, {"suzanne_texture_rough_met_ao", false} });
	Object dragon(Object::Type::Regular, "dragon", { { "dragon_texture_color", true }, { "dragon_texture_normal", false }, { "dragon_texture_rough_met_ao", false } });
	Object plane(Object::Type::Parallax, "groundplane", { { "groundplane_texture_color", true }, { "groundplane_texture_normal", false }, { "groundplane_texture_rough_met_ao", false }, { "groundplane_texture_depth", false } }, {}, false);
	
	suzanne.update(suzanneModel);
	dragon.update(dragonModel);
	plane.update(planeModel);
	
	objects.push_back(suzanne);
	objects.push_back(dragon);
	objects.push_back(plane);
	
	// Background creation.
	background = Object(Object::Type::Skybox, "skybox", {}, {{"corsica_beach_cube", true }});
	backgroundReflection = Resources::manager().getCubemap("corsica_beach_cube").id;
	loadSphericalHarmonics("corsica_beach_cube_shcoeffs");
	
	// Compute the bounding box of the shadow casters.
	const BoundingBox bbox = computeBoundingBox(true);
	
	// Lights creation.
	// Create directional light.
	directionalLights.emplace_back(glm::vec3(-2.0f,-1.5f,0.0f), glm::vec3(1.0f,1.0f, 0.92f), bbox);
	directionalLights[0].castShadow(true);
	// Create spotlight.
	spotLights.emplace_back(glm::vec3(2.0f,2.0f,2.0), glm::vec3(-1.0f,-1.0f,-1.0f), glm::vec3(0.0f,10.0f,10.0f), 0.5f, 0.6f, 5.0f, bbox);
	spotLights[0].castShadow(true);
	
	// Create point lights.
	const float lI = 4.0; // Light intensity.
	const std::vector<glm::vec3> colors = { glm::vec3(lI,0.0,0.0), glm::vec3(0.0,lI,0.0), glm::vec3(0.0,0.0,lI), glm::vec3(lI,lI,0.0)};
	for(size_t i = 0; i < 4; ++i){
		const glm::vec3 position = glm::vec3(-1.0f+2.0f*(i%2),-0.1f,-1.0f+2.0f*(i/2));
		pointLights.emplace_back(position, colors[i], 1.2f, bbox);
	}
}

void DragonScene::update(double fullTime, double frameTime){
	// Update lights.
	directionalLights[0].update(glm::vec3(-2.0f, -1.5f+sin(0.5*fullTime),0.0f));
	spotLights[0].update(glm::vec3(1.1f+sin(fullTime),2.0f,1.1f+sin(fullTime)));
	for(size_t i = 0; i <pointLights.size(); ++i){
		auto& pointLight = pointLights[i];
		const glm::vec4 newPosition = glm::rotate(glm::mat4(1.0f), (float)frameTime, glm::vec3(0.0f, 1.0f, 0.0f))*glm::vec4(pointLight.position(), 1.0f);
		pointLight.update(glm::vec3(newPosition));
	}
	
	// Update objects.
	const glm::mat4 suzanneModel = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.2,0.0,0.0)),float(fullTime),glm::vec3(0.0f,1.0f,0.0f)),glm::vec3(0.25f));
	objects[0].update(suzanneModel);
}


#endif
