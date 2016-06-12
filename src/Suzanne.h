#ifndef Suzanne_h
#define Suzanne_h
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <glm/glm.hpp>



class Suzanne {

public:

	Suzanne();

	~Suzanne();

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
	GLuint _texColor;
	GLuint _texNormal;
	GLuint _texEffects;
	GLuint _texCubeMap;
	GLuint _texCubeMapSmall;
	
	size_t _count;
	
	double _time;
	
	
};

#endif
