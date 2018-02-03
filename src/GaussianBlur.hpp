#ifndef GaussianBlur_h
#define GaussianBlur_h
#include "Blur.hpp"


class GaussianBlur : public Blur {

public:

	/// Init function
	GaussianBlur(int width, int height, int depth, GLuint format, GLuint type, GLuint preciseFormat);

	/// Draw function
	void process(const GLuint textureId);
	
	/// Clean function
	void clean() const;

	/// Handle screen resizing
	void resize(int width, int height);
	
private:
	
	ScreenQuad _blurScreen;
	std::shared_ptr<Framebuffer> _finalFramebuffer;
	ScreenQuad _combineScreen;
	std::vector<std::shared_ptr<Framebuffer>> _frameBuffers;
	std::vector<std::shared_ptr<Framebuffer>> _frameBuffersBlur;
	
};

#endif
