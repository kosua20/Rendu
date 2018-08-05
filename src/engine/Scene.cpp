#include "Scene.hpp"
#include "helpers/Logger.hpp"

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

BoundingBox Scene::computeBoundingBox(bool onlyShadowCasters){
	BoundingBox bbox;
	if(objects.empty()){
		return bbox;
	}
	bool first = true;
	for(size_t oid = 0; oid < objects.size(); ++oid){
		if(onlyShadowCasters && !objects[oid].castsShadow()){
			continue;
		}
		if(first){
			first = false;
			bbox = objects[oid].getBoundingBox();
			continue;
		}
		bbox.merge(objects[oid].getBoundingBox());
	}
	Log::Info() << Log::Resources << "Scene bounding box: [" << bbox.minis << ", " << bbox.maxis << "]." << std::endl;
	return bbox;
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
