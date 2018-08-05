#ifndef DirectionalLight_h
#define DirectionalLight_h
#include "Light.hpp"
#include "../ScreenQuad.hpp"
#include "../Framebuffer.hpp"
#include "../Object.hpp"
#include "../processing/BoxBlur.hpp"

#include <memory>

class DirectionalLight : public Light {

public:
	
	DirectionalLight(const glm::vec3& worldDirection, const glm::vec3& color, const BoundingBox & sceneBox);
	
	void init(const std::map<std::string, GLuint>& textureIds);
	
	void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize = glm::vec2(0.0f)) const;
	
	void drawShadow(const std::vector<Object> & objects) const;
	
	void drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
	
	void clean() const;
	
	void update(const glm::vec3 & newDirection);
	
private:
	
	ScreenQuad _screenquad;
	std::shared_ptr<Framebuffer> _shadowPass;
	std::shared_ptr<BoxBlur> _blur;
	glm::mat4 _projectionMatrix;
	glm::mat4 _viewMatrix;
	glm::vec3 _lightDirection;
	
	BoundingBox _sceneBox;

};

#endif
