#ifndef BoxBlur_h
#define BoxBlur_h
#include "../Common.hpp"
#include "Blur.hpp"


class BoxBlur : public Blur {

public:
	
	BoxBlur(unsigned int width, unsigned int height, bool approximate, GLuint format, GLuint type, GLuint preciseFormat, GLuint wrapping);

	/// Draw function
	void process(const GLuint textureId);
	
	/// Clean function
	void clean() const;

	/// Handle screen resizing
	void resize(unsigned int width, unsigned int height);

private:
	
	std::shared_ptr<ProgramInfos> _blurProgram;
	std::shared_ptr<Framebuffer> _finalFramebuffer;
	
};

#endif
