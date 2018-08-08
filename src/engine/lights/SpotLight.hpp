#ifndef SpotLight_h
#define SpotLight_h

#include "Light.hpp"
#include "../Common.hpp"
#include "../ScreenQuad.hpp"
#include "../Framebuffer.hpp"
#include "../Object.hpp"
#include "../processing/BoxBlur.hpp"


class SpotLight : public Light {

public:
	
	SpotLight(const glm::vec3& worldPosition, const glm::vec3& worldDirection, const glm::vec3& color, const float innerAngle, const float outerAngle, const float radius, const BoundingBox & sceneBox);
	
	void init(const std::map<std::string, GLuint>& textureIds);
	
	void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize = glm::vec2(0.0f)) const;
	
	void drawShadow(const std::vector<Object> & objects) const;
	
	void drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
	
	void clean() const;
	
	void update(const glm::vec3 & newPosition);
	
	void update(const glm::vec3 & newPosition, const glm::vec3 & newDirection);
	
	glm::vec3 position() const { return _lightPosition; }
	
private:
	
	std::vector<GLuint> _textureIds;
	std::shared_ptr<ProgramInfos> _program;
	MeshInfos _cone;
	std::shared_ptr<Framebuffer> _shadowPass;
	std::shared_ptr<BoxBlur> _blur;
	
	glm::mat4 _projectionMatrix;
	glm::mat4 _viewMatrix;
	glm::vec3 _lightDirection;
	glm::vec3 _lightPosition;
	float _innerHalfAngle, _outerHalfAngle, _radius;
	
	BoundingBox _sceneBox;
	std::shared_ptr<ProgramInfos> _programDepth;
	
};

#endif
