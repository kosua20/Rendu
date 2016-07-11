#ifndef Dragon_h
#define Dragon_h
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <glm/glm.hpp>



class Dragon {

public:

	Dragon();

	~Dragon();

	/// Init function
	void init();

	/// Draw function
	void draw(float elapsed, const glm::mat4& view, const glm::mat4& projection, const size_t pingpong);
	
	/// Draw depth function
	void drawDepth(float elapsed, const glm::mat4& view, const glm::mat4& projection);
	
	/// Clean function
	void clean();


private:
	
	GLuint _programId;
	GLuint _programDepthId;
	GLuint _vao;
	GLuint _ebo;
	GLuint _texColor;
	GLuint _texNormal;
	GLuint _texEffects;
	GLuint _lightUniformId;
	
	size_t _count;
	
	
	GLuint _texCubeMap;
	GLuint _texCubeMapSmall;
};

#endif
