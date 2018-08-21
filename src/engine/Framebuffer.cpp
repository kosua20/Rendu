#include "Framebuffer.hpp"
#include "helpers/GLUtilities.hpp"



Framebuffer::Framebuffer(unsigned int width, unsigned int height, GLuint format, GLuint type, GLuint preciseFormat, GLuint filtering, GLuint wrapping, bool depthBuffer) {
	_width = width;
	_height = height;
	_format = format;
	_type = type;
	_preciseFormat = preciseFormat;
	_useDepth = depthBuffer;

	// Create a framebuffer.
	glGenFramebuffers(1, &_id);
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
	
	// Create the.texture to store the result.
	glGenTextures(1, &_idColor);
	glBindTexture(GL_TEXTURE_2D, _idColor);
	glTexImage2D(GL_TEXTURE_2D, 0, preciseFormat, (GLsizei)_width , (GLsizei)_height, 0, format, type, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)filtering);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)filtering);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)wrapping);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)wrapping);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	if(wrapping == GL_CLAMP_TO_BORDER){
		// Setup the border value for the shadow map
		GLfloat border[] = { 1.0, 1.0, 1.0, 1.0 };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	}
	
	// Link the texture to the first color attachment (ie output) of the framebuffer.
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 ,GL_TEXTURE_2D, _idColor, 0);
	
	if (_useDepth) {
		// Create the renderbuffer (depth buffer).
		glGenRenderbuffers(1, &_idRenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, _idRenderbuffer);
		// Setup the depth buffer storage.
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, (GLsizei)_width, (GLsizei)_height);
		// Link the renderbuffer to the framebuffer.
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _idRenderbuffer);
	}

	//Register which color attachments to draw to.
	GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers);
	checkGLFramebufferError();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	checkGLError();
}

Framebuffer::~Framebuffer(){ clean(); }

void Framebuffer::bind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
}

void Framebuffer::setViewport() const
{
	glViewport(0, 0, (GLsizei)_width, (GLsizei)_height);
}

void Framebuffer::unbind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



void Framebuffer::resize(unsigned int width, unsigned int height){
	_width = width;
	_height = height;
	// Resize the renderbuffer.
	if (_useDepth) {
		glBindRenderbuffer(GL_RENDERBUFFER, _idRenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, (GLsizei)_width, (GLsizei)_height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
	// Resize the texture.
	glBindTexture(GL_TEXTURE_2D, _idColor);
	glTexImage2D(GL_TEXTURE_2D, 0, _preciseFormat, (GLsizei)_width, (GLsizei)_height, 0, _format, _type, 0);
}

void Framebuffer::resize(glm::vec2 size){
	resize((unsigned int)size[0], (unsigned int)size[1]);
}

void Framebuffer::clean() const {
	if (_useDepth) {
		glDeleteRenderbuffers(1, &_idRenderbuffer);
	}
	glDeleteTextures(1, &_idColor);
	glDeleteFramebuffers(1, &_id);
}

