#ifndef Scene_h
#define Scene_h
#include "Common.hpp"
#include "Object.hpp"
#include "lights/DirectionalLight.hpp"
#include "lights/PointLight.hpp"
#include "lights/SpotLight.hpp"
#include "resources/ResourcesManager.hpp"
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
	std::vector<SpotLight> spotLights;
	
protected:
	
	void loadSphericalHarmonics(const std::string & name);
	
	BoundingBox computeBoundingBox(bool onlyShadowCasters = false);
	
	bool _loaded = false;

};
#endif
