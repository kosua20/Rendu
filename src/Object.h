#ifndef Object_h
#define Object_h
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <glm/glm.hpp>



class Object {

public:

	Object();

	~Object();

	/// Init function
	void init(const std::string& meshPath, const std::vector<std::string>& texturesPaths, int materialId);

	/// Draw function
	void draw(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection);
	
	/// Draw depth function
	void drawDepth(const glm::mat4& model, const glm::mat4& vp);
	
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
	
	GLsizei _count;
	
	
	glm::mat4 _lightMVP;
	
};

#endif
