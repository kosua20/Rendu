#ifndef GaussianBlur_h
#define GaussianBlur_h
#include "../Common.hpp"
#include "Blur.hpp"


class GaussianBlur : public Blur {

public:

	/// Init function
	GaussianBlur(unsigned int width, unsigned int height, unsigned int depth, GLuint format, GLuint type, GLuint preciseFormat);

	/// Draw function
	void process(const GLuint textureId);
	
	/// Clean function
	void clean() const;

	/// Handle screen resizing
	void resize(unsigned int width, unsigned int height);
	
private:
	
	std::shared_ptr<ProgramInfos> _blurProgram;
	std::shared_ptr<Framebuffer> _finalFramebuffer;
	std::shared_ptr<ProgramInfos> _combineProgram;
	std::vector<std::shared_ptr<Framebuffer>> _frameBuffers;
	std::vector<std::shared_ptr<Framebuffer>> _frameBuffersBlur;
	std::vector<GLuint> _textures;
	
};

#endif
