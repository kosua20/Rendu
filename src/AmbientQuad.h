#ifndef AmbientQuad_h
#define AmbientQuad_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include "ScreenQuad.h"

class AmbientQuad : public ScreenQuad {

public:

	AmbientQuad();

	~AmbientQuad();
	
	void init(std::map<std::string, GLuint> textureIds, const std::string & shaderRoot);
	
	/// Draw function,
	void draw(const glm::vec2& invScreenSize, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

	
private:
	
	GLuint _texCubeMap;
	GLuint _texCubeMapSmall;
};

#endif
