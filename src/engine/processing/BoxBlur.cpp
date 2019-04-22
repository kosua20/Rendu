#include "BoxBlur.hpp"


BoxBlur::BoxBlur(unsigned int width, unsigned int height, bool approximate, const Descriptor & descriptor) : Blur() {
	
	Descriptor linearDescriptor = descriptor;
	linearDescriptor.filtering = GL_LINEAR;
	GLenum format, type;
	const int channels = GLUtilities::getTypeAndFormat(linearDescriptor.typedFormat, type, format);
	
	std::string blur_type_name = "box-blur-" + (approximate ? std::string("approx-") : "");
	blur_type_name.append(std::to_string(channels));
	
	_blurProgram = Resources::manager().getProgram2D(blur_type_name);
	// Create one framebuffer.
	_finalFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(width, height, linearDescriptor, false));
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
