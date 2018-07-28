#ifndef PointLight_h
#define PointLight_h
#include "Light.hpp"
#include "../resources/ResourcesManager.hpp"

class PointLight : public Light {

public:
	
	PointLight(const glm::vec3& worldPosition, const glm::vec3& color, float radius);
	
	void init(const std::map<std::string, GLuint>& textureIds);
	
	void draw( const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize ) const;
	
	void drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
	
	void clean() const;
	
	void update(const glm::vec3 & newPosition);
	
	glm::vec3 position() const { return _lightPosition; }
	
	static void loadProgramAndGeometry();
	
private:
	
	float _radius;
	std::vector<GLuint> _textureIds;
	
	std::shared_ptr<ProgramInfos> _program;
	//glm::mt4 _view;
	glm::vec3 _lightPosition;
	
	static std::shared_ptr<ProgramInfos> _debugProgram;
	static MeshInfos _debugMesh;
	
};

#endif
