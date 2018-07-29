#ifndef Scene_h
#define Scene_h
#include "Object.hpp"
#include "lights/DirectionalLight.hpp"
#include "lights/PointLight.hpp"
#include "resources/ResourcesManager.hpp"
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>
#include <sstream>

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
	std::vector<glm::vec3> backgroundIrradiance;
	GLuint backgroundReflection;
	std::vector<DirectionalLight> directionalLights;
	std::vector<PointLight> pointLights;
	
protected:
	
	void loadSphericalHarmonics(const std::string & name);
	
	bool _loaded = false;

};
#endif
