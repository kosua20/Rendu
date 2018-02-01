#ifndef Scene_h
#define Scene_h
#include "../Object.hpp"
#include "../lights/DirectionalLight.hpp"
#include "../lights/PointLight.hpp"
#include "../helpers/ResourcesManager.hpp"
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include <sstream>
#include <iostream>

class Scene {

public:

	~Scene();

	Scene();
	
	/// Init function
	virtual void init() = 0;
	
	virtual void update(double fullTime, double frameTime) = 0;
	
	void loadSphericalHarmonics(const std::string & name);
	
	/// Clean function
	void clean() const;
	
	std::vector<Object> objects;
	Object background;
	std::vector<glm::vec3> backgroundIrradiance;
	GLuint backgroundReflection;
	std::vector<DirectionalLight> directionalLights;
	std::vector<PointLight> pointLights;
	

};


inline Scene::Scene(){};

inline Scene::~Scene(){};

inline void Scene::loadSphericalHarmonics(const std::string & name){
	backgroundIrradiance.clear();
	backgroundIrradiance.resize(9);
	
	const std::string coeffsRaw = Resources::manager().getTextFile(name);
	std::stringstream coeffsStream(coeffsRaw);
	
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	
	for(int i = 0; i < 9; ++i){
		coeffsStream >> x >> y >> z;
		backgroundIrradiance[i] = glm::vec3(x,y,z);
	}
	
}

inline void Scene::clean() const {
	for(auto & object : objects){
		object.clean();
	}
	background.clean();
	for(auto& dirLight : directionalLights){
		dirLight.clean();
	}
};

#endif
