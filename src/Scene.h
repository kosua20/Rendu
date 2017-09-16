#ifndef Scene_h
#define Scene_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>

#include "Object.h"
#include "lights/DirectionalLight.h"
#include "lights/PointLight.h"

class Scene {

public:

	~Scene();

	/// Init function
	Scene();
	
	void init();
	
	void update(double timer, double elapsedTime);

	/// Clean function
	void clean() const;
	
	std::vector<Object> objects;
	Object background;
	std::vector<DirectionalLight> directionalLights;
	std::vector<PointLight> pointLights;
	
private:
	

	

};

#endif
