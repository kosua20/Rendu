#ifndef Scene_h
#define Scene_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>

#include "../Object.h"
#include "../lights/DirectionalLight.h"
#include "../lights/PointLight.h"

class Scene {

public:

	~Scene();

	Scene();
	
	/// Init function
	virtual void init() = 0;
	
	virtual void update(double fullTime, double frameTime) = 0;

	/// Clean function
	void clean() const;
	
	std::vector<Object> objects;
	Object background;
	GLuint backgroundIrradiance;
	GLuint backgroundReflection;
	std::vector<DirectionalLight> directionalLights;
	std::vector<PointLight> pointLights;
	

};


inline Scene::Scene(){};

inline Scene::~Scene(){};

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
