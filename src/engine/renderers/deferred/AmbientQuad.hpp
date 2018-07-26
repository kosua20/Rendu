#ifndef AmbientQuad_h
#define AmbientQuad_h
#include "../../ScreenQuad.hpp"

#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>

class AmbientQuad : public ScreenQuad {

public:

	AmbientQuad();

	~AmbientQuad();
	
	void init(std::map<std::string, GLuint> textureIds, const GLuint reflection, const std::vector<glm::vec3> & irradiance);
	
	/// Draw function,
	void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
	
	void drawSSAO(const glm::mat4& projectionMatrix) const;
		
	void clean() const;
	
private:
	
	GLuint setupSSAO();
	
	GLuint _texCubeMap;
	GLuint _texBrdfPrecalc;
	
	ScreenQuad _ssaoScreen;
	
	std::vector<glm::vec3> _samples;
	
};

#endif
