#ifndef ScreenQuad_h
#define ScreenQuad_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>

class ScreenQuad {

public:

	ScreenQuad();

	~ScreenQuad();

	/// Init function
	void init(GLuint textureId, const std::string & shaderRoot);
	
	void init(std::map<std::string, GLuint> textureIds, const std::string & shaderRoot);

	/// Draw function,
	void draw(const glm::vec2& invScreenSize);

	/// Clean function
	void clean();

	
protected:
	
	void loadGeometry();
	
	GLuint _programId;
	GLuint _vao;
	GLuint _ebo;
	std::vector<GLuint> _textureIds;
	

};

#endif
