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
	
	void update(float time, const glm::mat4& camViewMatrix);
	
	virtual void init(std::map<std::string, GLuint> textureIds)=0;
	
	virtual void draw(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) =0;
	
	glm::mat4 _mvp;
	glm::vec3 _color;
	
protected:
	
	glm::mat4 _projectionMatrix;
	glm::mat4 _viewMatrix;
	glm::vec3 _local;
	
};


inline Light::Light(const glm::vec3& worldPosition, const glm::vec3& color, const glm::mat4& projection){
	
	_local = worldPosition;
	_color = color;
	_projectionMatrix = projection;
	_viewMatrix = glm::lookAt(_local, glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f));
	_mvp = _projectionMatrix * _viewMatrix;
	
}


inline void Light::update(float time, const glm::mat4& camViewMatrix){
	
	_local = glm::vec3(2.0f,(1.5f + sin(0.5*time)),2.0f);
	_viewMatrix = glm::lookAt(_local, glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f));
	_mvp = _projectionMatrix * _viewMatrix;
	
}

#endif


