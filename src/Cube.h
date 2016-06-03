#ifndef Cube_h
#define Cube_h
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <glm/glm.hpp>



class Cube {

public:

	Cube();

	~Cube();

	/// Init function
	void init();

	/// Draw function
	void draw(float elapsed, const glm::mat4& view, const glm::mat4& projection);

	/// Clean function
	void clean();


private:
	
	GLuint _programId;
	GLuint _vao;
	GLuint _ebo;
	GLuint _texCubeMap;
	
	size_t _count;

};

#endif
