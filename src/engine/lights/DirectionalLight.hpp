#ifndef DirectionalLight_h
#define DirectionalLight_h
#include "Light.hpp"
#include "../ScreenQuad.hpp"
#include "../Framebuffer.hpp"
#include <memory>

class DirectionalLight : public Light {

public:
	
	DirectionalLight(const glm::vec3& worldPosition, const glm::vec3& color, const glm::mat4& projection = glm::mat4(1.0f));
	
	void init(const std::map<std::string, GLuint>& textureIds);
	
	void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize = glm::vec2(0.0f)) const;
	
	void bind() const;
	
	void blurAndUnbind() const;
	
	void clean() const;
	
private:
	
	ScreenQuad _screenquad;
	ScreenQuad _blurScreen;
	std::shared_ptr<Framebuffer> _shadowPass;
	std::shared_ptr<Framebuffer> _blurPass;
	
};

#endif
