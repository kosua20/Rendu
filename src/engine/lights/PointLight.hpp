#ifndef PointLight_h
#define PointLight_h
#include "Light.hpp"
#include "../resources/ResourcesManager.hpp"
#include "../FramebufferCube.hpp"
#include "../Object.hpp"

class PointLight : public Light {

public:
	
	PointLight(const glm::vec3& worldPosition, const glm::vec3& color, float radius, const BoundingBox & sceneBox);
	
	void init(const std::map<std::string, GLuint>& textureIds);
	
	void draw( const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize ) const;
	
	void drawShadow(const std::vector<Object> & objects) const;
	
	void drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
	
	void clean() const;
	
	void update(const glm::vec3 & newPosition);
	
	glm::vec3 position() const { return _lightPosition; }
	
private:
	
	float _radius;
	std::vector<GLuint> _textureIds;
	
	std::shared_ptr<ProgramInfos> _program;
	MeshInfos _sphere;
	glm::vec3 _lightPosition;
	
	BoundingBox _sceneBox;
	
	std::shared_ptr<ProgramInfos> _programDepth;
	std::shared_ptr<FramebufferCube> _shadowFramebuffer;
	std::vector<glm::mat4> _views;
	std::vector<glm::mat4> _mvps;
	float _farPlane;
};

#endif
