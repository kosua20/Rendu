#include "processing/BoxBlur.hpp"
#include "graphics/GLUtilities.hpp"

BoxBlur::BoxBlur(unsigned int width, unsigned int height, bool approximate, const Descriptor & descriptor) : Blur() {
	
	// Enforce linear filtering.
	Descriptor linearDescriptor(descriptor.typedFormat(), Filter::LINEAR_NEAREST, descriptor.wrapping());
	const int channels = linearDescriptor.getChannelsCount();
	
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
void BoxBlur::process(const Texture * textureId){
	_finalFramebuffer->bind();
	_finalFramebuffer->setViewport();
	glClear(GL_COLOR_BUFFER_BIT);
	_blurProgram->use();
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
