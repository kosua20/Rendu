#ifndef Light_h
#define Light_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <string>
#undef near
#undef far

class Light {

public:
	
	Light(const glm::vec3& color);
	
	void castShadow(const bool shouldCast){ _castShadows = shouldCast; }
	
	void setIntensity(const glm::vec3 & color){ _color = color; }
	
protected:
	
	glm::mat4 _mvp;
	glm::vec3 _color;
	bool _castShadows;
};


inline Light::Light(const glm::vec3& color){
	_castShadows = false;
	_color = color;
	_mvp = glm::mat4(1.0f);
}

#endif


