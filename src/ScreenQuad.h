#ifndef ScreenQuad_h
#define ScreenQuad_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include "helpers/ResourcesManager.h"

class ScreenQuad {

public:

	ScreenQuad();

	~ScreenQuad();

	/// Init function
	void init(const std::string & shaderRoot);
	
	void init(GLuint textureId, const std::string & shaderRoot);
	
	void init(std::map<std::string, GLuint> textureIds, const std::string & shaderRoot);

	/// Draw function,
	void draw() const;
	
	void draw(const glm::vec2& invScreenSize) const;
	
	void draw(GLuint textureId) const;
	
	void draw(GLuint textureId, const glm::vec2& invScreenSize) const;

	/// Clean function
	void clean() const;

	const std::shared_ptr<ProgramInfos> program() const { return _program; }
	
	std::shared_ptr<ProgramInfos> program() { return _program; }
	
protected:
	
	void loadGeometry();
	
	std::shared_ptr<ProgramInfos> _program;
	GLuint _vao;
	GLuint _ebo;
	std::vector<GLuint> _textureIds;
	

};

#endif
