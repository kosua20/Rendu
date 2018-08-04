#ifndef DirectionalLight_h
#define DirectionalLight_h
#include "Light.hpp"
#include "../ScreenQuad.hpp"
#include "../Framebuffer.hpp"
#include <memory>

class DirectionalLight : public Light {

public:
	
	DirectionalLight(const glm::vec3& worldDirection, const glm::vec3& color, const float extent, const float near, const float far);
	
	void init(const std::map<std::string, GLuint>& textureIds);
	
	void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize = glm::vec2(0.0f)) const;
	
	void drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
	
	void bind() const;
	
	void blurAndUnbind() const;
	
	void clean() const;
	
	void update(const glm::vec3 & newDirection);
	
	static void loadProgramAndGeometry();
	
private:
	
	ScreenQuad _screenquad;
	ScreenQuad _blurScreen;
	std::shared_ptr<Framebuffer> _shadowPass;
	std::shared_ptr<Framebuffer> _blurPass;
	glm::mat4 _projectionMatrix;
	glm::mat4 _viewMatrix;
	glm::vec3 _lightDirection;
	
	static std::shared_ptr<ProgramInfos> _debugProgram;
	static MeshInfos _debugMesh;
};

#endif
