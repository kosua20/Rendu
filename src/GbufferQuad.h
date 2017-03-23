#ifndef GbufferQuad_h
#define GbufferQuad_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include "ScreenQuad.h"

class GbufferQuad : public ScreenQuad {

public:

	GbufferQuad();

	~GbufferQuad();
	
	void init(std::map<std::string, GLuint> textureIds, const std::string & shaderRoot, GLuint shadowMapTextureId);
	
	/// Draw function,
	void draw(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::mat4& lightMatrix, GLuint pingpong);

	
private:
	
	GLuint _lightUniformId;
	GLuint _texCubeMap;
	GLuint _texCubeMapSmall;
	GLuint _shadowMapId;
};

#endif
