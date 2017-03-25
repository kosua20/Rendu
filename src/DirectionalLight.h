#ifndef DirectionalLight_h
#define DirectionalLight_h
#include "Light.h"
#include "ScreenQuad.h"

class DirectionalLight : public Light {

public:
	
	DirectionalLight(const glm::vec3& worldPosition, const glm::vec3& color, const glm::mat4& projection = glm::mat4(1.0f));
	
	void init(std::map<std::string, GLuint> textureIds);
	
	void draw(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
		
private:
	
	ScreenQuad _screenquad;
};

#endif
