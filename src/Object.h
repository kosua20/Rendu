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
	void init(const std::string& meshPath, const std::vector<std::string>& texturesPaths, GLuint shadowMapTextureId, int materialId);

	/// Draw function
	void draw(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, const size_t pingpong);
	
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
	GLuint _texCubeMap;
	GLuint _texCubeMapSmall;
	GLuint _lightUniformId;
	GLuint _shadowMapId;
	
	size_t _count;
	
	
	glm::mat4 _lightMVP;
	
};

#endif
