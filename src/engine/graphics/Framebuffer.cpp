#include "graphics/Framebuffer.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GLUtilities.hpp"

Framebuffer::Framebuffer(uint width, uint height, const Descriptor & descriptor, bool depthBuffer) :
	Framebuffer(TextureShape::D2, width, height, 1, 1, std::vector<Descriptor>(1, descriptor), depthBuffer) {
}

Framebuffer::Framebuffer(uint width, uint height, const std::vector<Descriptor> & descriptors, bool depthBuffer) :
	Framebuffer(TextureShape::D2, width, height, 1, 1, descriptors, depthBuffer) {
}

Framebuffer::Framebuffer(TextureShape shape, uint width, uint height, uint depth, uint mips, const std::vector<Descriptor> & descriptors, bool depthBuffer) :
	_width(width), _height(height) {

	// Check that the shape is supported.
	_shape = shape;
	if(_shape != TextureShape::D2 && _shape != TextureShape::Array2D && _shape != TextureShape::Cube && _shape != TextureShape::ArrayCube){
		Log::Error() << Log::OpenGL << "Unsupported framebuffer shape." << std::endl;
		return;
	}
	if(shape == TextureShape::D2){
		_depth = 1;
	} else if(shape == TextureShape::Cube){
		_depth = 6;
	} else if(shape == TextureShape::ArrayCube){
		_depth = 6 * depth;
	} else {
		_depth = depth;
	}
	_depthUse = Depth::NONE;
	_target = GLUtilities::targetFromShape(_shape);
	// Create a framebuffer.
	glGenFramebuffers(1, &_id);
	glBindFramebuffer(GL_FRAMEBUFFER, _id);

	for(const auto & descriptor : descriptors) {
		// Create the color texture to store the result.
		const Layout & format		  = descriptor.typedFormat();
		const bool isDepthComp		  = format == Layout::DEPTH_COMPONENT16 || format == Layout::DEPTH_COMPONENT24 || format == Layout::DEPTH_COMPONENT32F;
		const bool isDepthStencilComp = format == Layout::DEPTH24_STENCIL8 || format == Layout::DEPTH32F_STENCIL8;

		if(isDepthComp || isDepthStencilComp) {
			_depthUse		= Depth::TEXTURE;
			_idDepth.width  = _width;
			_idDepth.height = _height;
			_idDepth.depth  = 1;
			_idDepth.levels = mips;
			// For now we don't support layered rendering, depth is always a TEXTURE_2D.
			_idDepth.shape  = TextureShape::D2;
			GLUtilities::setupTexture(_idDepth, descriptor);

			// Link the texture to the depth attachment of the framebuffer.
			glBindTexture(GL_TEXTURE_2D, _idDepth.gpu->id);
			glFramebufferTexture2D(GL_FRAMEBUFFER, (isDepthStencilComp ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT), GL_TEXTURE_2D, _idDepth.gpu->id, 0);
			glBindTexture(GL_TEXTURE_2D, 0);

		} else {
			_idColors.emplace_back();
			Texture & tex = _idColors.back();
			tex.width     = _width;
			tex.height	  = _height;
			tex.depth	  = _depth;
			tex.levels	  = mips;
			tex.shape	  = shape;
			GLUtilities::setupTexture(tex, descriptor);

			// Link the texture to the color attachment (ie output) of the framebuffer.
			glBindTexture(_target, tex.gpu->id); // might not be needed.
			const GLuint slot = GLuint(int(_idColors.size()) - 1);
			// Two cases: 2D texture or array (either 2D array, cubemap, or cubemap array).
			if(_shape == TextureShape::D2){
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, GL_TEXTURE_2D, tex.gpu->id, 0);
			} else if(_shape == TextureShape::Cube){
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, GL_TEXTURE_CUBE_MAP_POSITIVE_X, tex.gpu->id, 0);
			} else {
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, tex.gpu->id, 0, 0);
			}
			glBindTexture(_target, 0);
			checkGLError();
		}
	}
	// If the depth buffer has not been setup yet but we require it, use a renderbuffer.
	if(_depthUse == Depth::NONE && depthBuffer) {
		_idDepth.width  = _width;
		_idDepth.height = _height;
		_idDepth.levels = 1;
		_idDepth.shape  = TextureShape::D2;
		_idDepth.gpu.reset(new GPUTexture(Descriptor(), _idDepth.shape));
		// Create the renderbuffer (depth buffer).
		glGenRenderbuffers(1, &_idDepth.gpu->id);
		glBindRenderbuffer(GL_RENDERBUFFER, _idDepth.gpu->id);
		// Setup the depth buffer storage.
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, GLsizei(_width), GLsizei(_height));
		// Link the renderbuffer to the framebuffer.
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _idDepth.gpu->id);
		_depthUse = Depth::RENDERBUFFER;
	}
	//Register which color attachments to draw to.
	std::vector<GLenum> drawBuffers(_idColors.size());
	for(size_t i = 0; i < _idColors.size(); ++i) {
		drawBuffers[i] = GL_COLOR_ATTACHMENT0 + GLuint(i);
	}
	glDrawBuffers(GLsizei(drawBuffers.size()), &drawBuffers[0]);
	checkGLFramebufferError();
	checkGLError();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::bind() const {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _id);
}

void Framebuffer::bind(Mode mode) const {
	if(mode == Mode::READ){
		glBindFramebuffer(GL_READ_FRAMEBUFFER, _id);
	} else if(mode == Mode::WRITE){
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _id);
	} else if(mode == Mode::SRGB){
		// When mode is SRGB, we assume we want to write.
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _id);
		glEnable(GL_FRAMEBUFFER_SRGB);
	}
}

void Framebuffer::bind(size_t layer, Mode mode) const {
	// When mode is SRGB, we assume we want to write.
	const GLenum target = mode == Mode::READ ? GL_READ_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER;
	glBindFramebuffer(target, _id);
	// Bind the proper slice for each color attachment.
	for(uint cid = 0; cid < _idColors.size(); ++cid){
		glBindTexture(_target, _idColors[cid].gpu->id);
		const GLenum slot = GL_COLOR_ATTACHMENT0 + GLuint(cid);
		const GLuint id = _idColors[cid].gpu->id;
		if(_shape == TextureShape::D2){
			glFramebufferTexture2D(target, slot, GL_TEXTURE_2D, id, 0);
		} else if(_shape == TextureShape::Cube){
			glFramebufferTexture2D(target, slot, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer), id, 0);
		} else {
			glFramebufferTextureLayer(target, slot, id, 0, GLint(layer));
		}
		glBindTexture(_target, 0);
	}
	if(mode == Mode::SRGB){
		glEnable(GL_FRAMEBUFFER_SRGB);
	}
}

void Framebuffer::setViewport() const {
	GLUtilities::setViewport(0, 0, int(_width), int(_height));
}

void Framebuffer::unbind() const {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glDisable(GL_FRAMEBUFFER_SRGB);
}

void Framebuffer::resize(uint width, uint height) {
	_width  = width;
	_height = height;

	// Resize the renderbuffer.
	if(_depthUse == Depth::RENDERBUFFER) {
		_idDepth.width  = _width;
		_idDepth.height = _height;
		glBindRenderbuffer(GL_RENDERBUFFER, _idDepth.gpu->id);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, GLsizei(_width), GLsizei(_height));
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

	} else if(_depthUse == Depth::TEXTURE) {
		_idDepth.width  = _width;
		_idDepth.height = _height;
		GLUtilities::allocateTexture(_idDepth);
	}

	// Resize the textures.
	for(auto & idColor : _idColors) {
		idColor.width  = _width;
		idColor.height = _height;
		GLUtilities::allocateTexture(idColor);
	}
}

void Framebuffer::resize(const glm::ivec2 & size) {
	resize(uint(size[0]), uint(size[1]));
}

void Framebuffer::clear(const glm::vec4 & color, float depth){
	// Clear depth.
	bind(0, Mode::WRITE);
	glClearBufferfv(GL_DEPTH, 0, &depth);
	for(uint lid = 0; lid < _depth; ++lid){
		bind(lid, Mode::WRITE);
		for(uint cid = 0; cid < _idColors.size(); ++cid){
			glClearBufferfv(GL_COLOR, GLint(cid), &color[0]);
		}
	}

}

void Framebuffer::clean() {
	if(_depthUse == Depth::RENDERBUFFER) {
		glDeleteRenderbuffers(1, &_idDepth.gpu->id);
	} else if(_depthUse == Depth::TEXTURE) {
		_idDepth.clean();
	}
	for(Texture & idColor : _idColors) {
		idColor.clean();
	}
	glDeleteFramebuffers(1, &_id);
}

glm::vec3 Framebuffer::read(const glm::ivec2 & pos) const {
	glm::vec3 rgb(0.0f);
	bind(0, Mode::READ);
	glReadPixels(pos.x, pos.y, 1, 1, GL_RGB, GL_FLOAT, &rgb[0]);
	unbind();
	return rgb;
}

Framebuffer * Framebuffer::_backbuffer = nullptr;

const Framebuffer * Framebuffer::backbuffer() {
	// Initialize a dummy framebuffer representing the backbuffer.
	if(!_backbuffer) {
		_backbuffer = new Framebuffer();
		_backbuffer->_idColors.emplace_back();
		// We don't really need to allocate the texture, just setup its descriptor.
		Texture & tex = _backbuffer->_idColors.back();
		tex.shape  = TextureShape::D2;
		tex.levels = 1;
		tex.depth  = 1;
		tex.gpu.reset(new GPUTexture(Descriptor(Layout::SRGB8_ALPHA8, Filter::NEAREST, Wrap::CLAMP), tex.shape));
	}
	return _backbuffer;
}
