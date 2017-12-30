#ifndef Light_h
#define Light_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <string>


class Light {

public:
	
	Light(const glm::vec3& worldPosition, const glm::vec3& color, const glm::mat4& projection = glm::mat4(1.0f));
	
	void update(const glm::vec3& worldPosition);
	
	virtual void init(const std::map<std::string, GLuint>& textureIds) =0;
	
	virtual void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize) const =0;
	
	virtual void clean() const =0;
	
	const glm::mat4 mvp() const { return _mvp; }
	const glm::vec3 local() const { return _local; }
	
protected:
	
	glm::mat4 _projectionMatrix;
	glm::mat4 _viewMatrix;
	glm::mat4 _mvp;
	glm::vec3 _color;
	glm::vec3 _local;
	
};


inline Light::Light(const glm::vec3& worldPosition, const glm::vec3& color, const glm::mat4& projection){
	
	_local = worldPosition;
	_color = color;
	_projectionMatrix = projection;
	_viewMatrix = glm::lookAt(_local, glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f));
	_mvp = _projectionMatrix * _viewMatrix;
	
}


inline void Light::update(const glm::vec3& worldPosition){
	
	_local = worldPosition;
	_viewMatrix = glm::lookAt(_local, glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f));
	_mvp = _projectionMatrix * _viewMatrix;
	
}

#endif


