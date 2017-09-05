#ifndef Blur_h
#define Blur_h
#include <gl3w/gl3w.h>
#include <glm/glm.hpp>
#include <memory>

#include "Framebuffer.h"
#include "ScreenQuad.h"

class Blur {

public:

	~Blur();

	/// Init function
	Blur(int width, int height, int depth);

	/// Draw function
	void process(const GLuint textureId);
	
	void draw();
	
	/// Clean function
	void clean() const;

	/// Handle screen resizing
	void resize(int width, int height);

	
private:
	
	ScreenQuad _passthrough;
	std::vector<std::shared_ptr<Framebuffer>> _frameBuffers;

};

#endif
