#include "BoxBlur.hpp"


BoxBlur::BoxBlur(unsigned int width, unsigned int height, bool approximate, const Framebuffer::Descriptor & descriptor) : Blur() {
	
	Framebuffer::Descriptor linearDescriptor = descriptor;
	linearDescriptor.filtering = GL_LINEAR;
	GLenum format, type;
	GLUtilities::getTypeAndFormat(linearDescriptor.typedFormat, type, format);
	
	std::string blur_type_name = "box-blur-" + (approximate ? std::string("approx-") : "");
	switch (format) {
		case GL_RED:
			blur_type_name += "1";
			break;
		case GL_RG:
			blur_type_name += "2";
			break;
		case GL_RGB:
			blur_type_name += "3";
			break;
		default:
			blur_type_name += "4";
			break;
	}
	_blurProgram = Resources::manager().getProgram2D(blur_type_name);
	// Create one framebuffer.
	_finalFramebuffer = std::make_shared<Framebuffer>(width, height, linearDescriptor, false);
	// Final combining buffer.
	_finalTexture = _finalFramebuffer->textureId();
	
	checkGLError();
	
}

// Draw function
void BoxBlur::process(const GLuint textureId){
	_finalFramebuffer->bind();
	_finalFramebuffer->setViewport();
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(_blurProgram->id());
	ScreenQuad::draw(textureId);
	_finalFramebuffer->unbind();
}


// Clean function
void BoxBlur::clean() const {
	_finalFramebuffer->clean();
	Blur::clean();
}

// Handle screen resizing
void BoxBlur::resize(unsigned int width, unsigned int height){
	_finalFramebuffer->resize(width, height);
}

void BoxBlur::clear(){
	_finalFramebuffer->bind();
	_finalFramebuffer->setViewport();
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	_finalFramebuffer->unbind();
}
