#include "graphics/FramebufferCube.hpp"
#include "graphics/GLUtilities.hpp"

FramebufferCube::FramebufferCube(unsigned int side, const Descriptor & descriptor, CubeMode mode, bool depthBuffer) :
	_side(side), _id(0), _useDepth(depthBuffer) {

	// Create a framebuffer.
	glGenFramebuffers(1, &_id);
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
	// Create the texture to store the result.
	_idColor.width  = _side;
	_idColor.height = _side;
	_idColor.depth  = 6;
	_idColor.levels = 1;
	_idColor.shape  = TextureShape::Cube;
	GLUtilities::setupTexture(_idColor, descriptor);

	// Link the texture to the first color attachment (ie output) of the framebuffer.
	glBindTexture(GL_TEXTURE_CUBE_MAP, _idColor.gpu->id);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _idColor.gpu->id, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	if(_useDepth) {
		_idDepth.width  = _side;
		_idDepth.height = _side;
		_idDepth.levels = 1;

		if(mode == CubeMode::COMBINED) {
			// Either use a cubemap for combined rendering (using a geometry shader to output to different layers).
			_idDepth.depth = 6;
			_idDepth.shape = TextureShape::Cube;
		} else {
			// Or work slice-by-slice, using a 2D depth buffer.
			_idDepth.depth = 1;
			_idDepth.shape = TextureShape::D2;
		}

		GLUtilities::setupTexture(_idDepth, {Layout::DEPTH_COMPONENT32F, Filter::NEAREST, Wrap::CLAMP});

		glBindTexture(_idDepth.gpu->target, _idDepth.gpu->id);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _idDepth.gpu->id, 0);
		glBindTexture(_idDepth.gpu->target, 0);
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

void FramebufferCube::bind(size_t slice) const {
	bind();
	// Bind the proper slice as the first color attachment.
	glBindTexture(GL_TEXTURE_CUBE_MAP, _idColor.gpu->id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + glm::clamp(slice, size_t(0), size_t(5))), _idColor.gpu->id, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void FramebufferCube::setViewport() const {
	GLUtilities::setViewport(0, 0, int(_side), int(_side));
}

void FramebufferCube::unbind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FramebufferCube::resize(unsigned int side) {
	_side = side;
	// Resize the renderbuffer.
	if(_useDepth) {
		_idDepth.width  = _side;
		_idDepth.height = _side;
		GLUtilities::allocateTexture(_idDepth);
	}
	// Resize the texture.
	_idColor.width  = _side;
	_idColor.height = _side;
	GLUtilities::allocateTexture(_idColor);
}

void FramebufferCube::clean() {
	if(_useDepth) {
		_idDepth.clean();
	}
	_idColor.clean();
	glDeleteFramebuffers(1, &_id);
}
