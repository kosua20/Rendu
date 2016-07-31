#ifndef ScreenQuad_h
#define ScreenQuad_h
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <glm/glm.hpp>


class ScreenQuad {

public:

	ScreenQuad();

	~ScreenQuad();

	/// Init function
	void init(GLuint textureId, const std::string & shaderRoot);

	/// Draw function
	void draw(glm::vec2 invScreenSize);

	/// Clean function
	void clean();

	void switchFXAA();
	
private:
	
	GLuint _programId;
	GLuint _vao;
	GLuint _ebo;
	GLuint _textureId;
	
	size_t _count;

};

#endif
