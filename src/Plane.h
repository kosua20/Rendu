#ifndef Plane_h
#define Plane_h
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <glm/glm.hpp>

class Plane {

public:

	Plane();

	~Plane();

	/// Init function
	void init();

	/// Draw function
	void draw(float elapsed, const glm::mat4& view, const glm::mat4& projection, size_t pingpong);

	/// Clean function
	void clean();


private:
	
	GLuint _programId;
	GLuint _vao;
	GLuint _ebo;
	GLuint _lightUniformId;
	
	size_t _count;

};

#endif
