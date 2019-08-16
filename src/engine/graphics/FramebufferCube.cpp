#include "graphics/FramebufferCube.hpp"
#include "graphics/GLUtilities.hpp"


FramebufferCube::FramebufferCube(unsigned int side, const Descriptor & descriptor, bool depthBuffer) {

	_side = side;
	_useDepth = depthBuffer;
	
	// Create a framebuffer.
	glGenFramebuffers(1, &_id);
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
	// Create the texture to store the result.
	_idColor.width = _side;
	_idColor.height = _side;
	_idColor.depth = 6;
	_idColor.levels = 1;
	_idColor.shape = TextureShape::Cube;
	GLUtilities::setupTexture(_idColor, descriptor);
	
	// Link the texture to the first color attachment (ie output) of the framebuffer.
	glBindTexture(GL_TEXTURE_CUBE_MAP, _idColor.gpu->id);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _idColor.gpu->id, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	
	if (_useDepth) {
		_idRenderbuffer.width = _side;
		_idRenderbuffer.height = _side;
		_idRenderbuffer.depth = 6;
		_idRenderbuffer.levels = 1;
		_idRenderbuffer.shape = TextureShape::Cube;
		GLUtilities::setupTexture(_idRenderbuffer, {DEPTH_COMPONENT32F, Filter::NEAREST, Wrap::CLAMP});
		
		glBindTexture(GL_TEXTURE_CUBE_MAP, _idRenderbuffer.gpu->id);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _idRenderbuffer.gpu->id, 0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}
	
	//Register which color attachments to draw to.
	GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers);
	checkGLFramebufferError();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	checkGLError();
}

void FramebufferCube::bind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
}

void FramebufferCube::setViewport() const {
	glViewport(0, 0, (GLsizei)_side, (GLsizei)_side);
}

void FramebufferCube::unbind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FramebufferCube::resize(unsigned int side){
	_side = side;
	// Resize the renderbuffer.
	if (_useDepth) {
		_idRenderbuffer.width = _side;
		_idRenderbuffer.height = _side;
		GLUtilities::allocateTexture(_idRenderbuffer);
	}
	// Resize the texture.
	_idColor.width = _side;
	_idColor.height = _side;
	GLUtilities::allocateTexture(_idColor);
}

void FramebufferCube::clean() {
	if (_useDepth) {
		_idRenderbuffer.clean();
	}
	_idColor.clean();
	glDeleteFramebuffers(1, &_id);
}

