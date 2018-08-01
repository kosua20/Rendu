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
	
	virtual void init(const std::map<std::string, GLuint>& textureIds) =0;
	
	virtual void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize) const =0;
	
	virtual void clean() const =0;
	
	const glm::mat4 mvp() const { return _mvp; }
	
protected:
	
	glm::mat4 _mvp;
	glm::vec3 _color;
	
};


inline Light::Light(const glm::vec3& color){
	
	_color = color;
	_mvp = glm::mat4(1.0f);
	
}

#endif


