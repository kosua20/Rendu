#ifndef Blur_h
#define Blur_h
#include "../Framebuffer.hpp"
#include "../ScreenQuad.hpp"

#include <gl3w/gl3w.h>
#include <glm/glm.hpp>
#include <memory>


class Blur {

public:

	~Blur();

	/// Draw function
	void process(const GLuint textureId);
	
	void draw();
	
	/// Clean function
	void clean() const;

	/// Handle screen resizing
	void resize(unsigned int width, unsigned int height);

	GLuint textureId() const;
	
protected:
	
	Blur();
	GLuint _finalTexture;
	ScreenQuad _passthrough;
	
};

#endif
