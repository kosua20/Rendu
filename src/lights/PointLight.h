#ifndef PointLight_h
#define PointLight_h
#include "Light.h"

class PointLight : public Light {

public:
	
	PointLight(const glm::vec3& worldPosition, const glm::vec3& color, float radius, const glm::mat4& projection = glm::mat4(1.0f));
	
	void init(const std::map<std::string, GLuint>& textureIds);
	
	void draw(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
	
	void drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
	
	void clean() const;
	
	static void loadProgramAndGeometry();
	
private:
	
	float _radius;
	std::vector<GLuint> _textureIds;
	
	GLuint _programId;
	GLuint _projId;
	GLuint _screenId;
	GLuint _radiusId;
	GLuint _positionId;
	GLuint _mvpId;
	GLuint _lightPosId;
	GLuint _lightColId;
	
	static GLuint _debugProgramId;
	static GLuint _ebo;
	static GLuint _vao;
	static GLsizei _count;
	
	
	
};

#endif
