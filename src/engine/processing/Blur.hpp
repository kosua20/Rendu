#ifndef Blur_h
#define Blur_h
#include "../Common.hpp"
#include "../Framebuffer.hpp"
#include "../ScreenQuad.hpp"


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
	std::shared_ptr<ProgramInfos> _passthroughProgram;
	
};

#endif
