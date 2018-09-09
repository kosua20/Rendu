#include "FramebufferCube.hpp"
#include "GLUtilities.hpp"


FramebufferCube::FramebufferCube(unsigned int side, const Framebuffer::Descriptor & descriptor, bool depthBuffer) {

	_descriptor = descriptor;
	_side = side;
	_useDepth = depthBuffer;
	
	GLenum type, format;
	GLUtilities::getTypeAndFormat(_descriptor.typedFormat, type, format);
	
	// Create a framebuffer.
	glGenFramebuffers(1, &_id);
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
	// Create the texture to store the result.
	glGenTextures(1, &_idColor);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _idColor);
	
	// Allocate all 6 layers.
	for(unsigned int i = 0; i < 6; ++i){
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, _descriptor.typedFormat, (GLsizei)_side , (GLsizei)_side, 0, format, type, 0);
	}
	
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, (GLint)_descriptor.filtering);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, (GLint)_descriptor.filtering);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// Link the texture to the first color attachment (ie output) of the framebuffer.
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _idColor, 0);
	if (_useDepth) {
		// Create the depth buffer.
		glGenTextures(1, &_idRenderbuffer);
		glBindTexture(GL_TEXTURE_CUBE_MAP, _idRenderbuffer);
		// Allocate all 6 layers.
		for(unsigned int i = 0; i < 6; ++i){
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, GL_DEPTH_COMPONENT32F, (GLsizei)_side , (GLsizei)_side, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _idRenderbuffer, 0);
	}
	
	//Register which color attachments to draw to.
	GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers);
	checkGLFramebufferError();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	checkGLError();
}

void FramebufferCube::bind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
}

void FramebufferCube::setViewport() const
{
	glViewport(0, 0, (GLsizei)_side, (GLsizei)_side);
}

void FramebufferCube::unbind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void FramebufferCube::resize(unsigned int side){
	_side = side;
	// Resize the renderbuffer.
	if (_useDepth) {
		glBindTexture(GL_TEXTURE_CUBE_MAP, _idRenderbuffer);
		// Allocate all 6 layers.
		for(unsigned int i = 0; i < 6; ++i){
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, GL_DEPTH_COMPONENT32F, (GLsizei)_side , (GLsizei)_side, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		}
	}
	// Resize the texture.
	GLenum type, format;
	GLUtilities::getTypeAndFormat(_descriptor.typedFormat, type, format);
	
	glBindTexture(GL_TEXTURE_CUBE_MAP, _idColor);
	// Reallocate all 6 layers.
	for(unsigned int i = 0; i < 6; ++i){
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, _descriptor.typedFormat, (GLsizei)_side , (GLsizei)_side, 0, format, type, 0);
	}
}


void FramebufferCube::clean() const {
	if (_useDepth) {
		glDeleteTextures(1, &_idRenderbuffer);
	}
	glDeleteTextures(1, &_idColor);
	glDeleteFramebuffers(1, &_id);
}

