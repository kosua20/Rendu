#include "Scene.hpp"

Scene::Scene(){};

Scene::~Scene(){};

void Scene::loadSphericalHarmonics(const std::string & name){
	backgroundIrradiance.clear();
	backgroundIrradiance.resize(9);
	
	const std::string coeffsRaw = Resources::manager().getString(name);
	std::stringstream coeffsStream(coeffsRaw);
	
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	
	for(int i = 0; i < 9; ++i){
		coeffsStream >> x >> y >> z;
		backgroundIrradiance[i] = glm::vec3(x,y,z);
	}
	
}

void Scene::clean() const {
	for(auto & object : objects){
		object.clean();
	}
	background.clean();
	for(auto& dirLight : directionalLights){
		dirLight.clean();
	}
	for(auto& pointLight : pointLights){
		pointLight.clean();
	}
	for(auto& spotLight : spotLights){
		spotLight.clean();
	}
};
