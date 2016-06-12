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
	void init(GLuint textureId);

	/// Draw function
	void draw(float time);

	/// Clean function
	void clean();


private:
	
	GLuint _programId;
	GLuint _vao;
	GLuint _ebo;
	GLuint _textureId;
	
	size_t _count;

};

#endif
