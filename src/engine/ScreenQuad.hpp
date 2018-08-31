#ifndef ScreenQuad_h
#define ScreenQuad_h

#include "Common.hpp"
#include "resources/ResourcesManager.hpp"

#include <map>


class ScreenQuad {

public:

	/// Draw function,
	static void draw();
	
	static void draw(GLuint textureId);
	
	static void draw(const std::vector<GLuint> & textureIds);
	
	
private:
	
	ScreenQuad();
	
	static GLuint _vao;
};

#endif
