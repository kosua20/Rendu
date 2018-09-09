#include "Framebuffer.hpp"
#include "GLUtilities.hpp"

Framebuffer::Framebuffer(unsigned int width, unsigned int height, const GLenum typedFormat, bool depthBuffer) : Framebuffer(width, height, {Descriptor(typedFormat)}, depthBuffer) {
	
}

Framebuffer::Framebuffer(unsigned int width, unsigned int height, const Descriptor & descriptor, bool depthBuffer) : Framebuffer(width, height, std::vector<Descriptor>(1, descriptor) , depthBuffer){
	
}

Framebuffer::Framebuffer(unsigned int width, unsigned int height, const std::vector<Descriptor> & descriptors, bool depthBuffer) {
	
	_width = width;
	_height = height;
	
	// Create a framebuffer.
	glGenFramebuffers(1, &_id);
	glBindFramebuffer(GL_FRAMEBUFFER, _id);
	bool depthBufferSetup = false;
	
	for(size_t i = 0; i < descriptors.size(); ++i){
		// Create the color texture to store the result.
		const auto & descriptor = descriptors[i];
		GLuint type, format;
		GLUtilities::getTypeAndFormat(descriptor.typedFormat, type, format);
		
		if(format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL){
			glGenTextures(1, &_idDepth);
			glBindTexture(GL_TEXTURE_2D, _idDepth);
			glTexImage2D(GL_TEXTURE_2D, 0, descriptor.typedFormat, (GLsizei)_width , (GLsizei)_height, 0, format, type, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)descriptor.filtering);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)descriptor.filtering);
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)descriptor.wrapping);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)descriptor.wrapping);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
			
			if(descriptor.wrapping == GL_CLAMP_TO_BORDER){
				// Setup the border value for the shadow map
				GLfloat border[] = { 1.0, 1.0, 1.0, 1.0 };
				glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
			}
			
			// Link the texture to the depth attachment of the framebuffer.
			glFramebufferTexture2D(GL_FRAMEBUFFER, (format == GL_DEPTH_STENCIL ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT), GL_TEXTURE_2D, _idDepth, 0);
			depthBufferSetup = true;
			_depthUse = TEXTURE;
			_depthDescriptor = descriptor;
			
		} else {
			GLuint idColor = 0;
			glGenTextures(1, &idColor);
			glBindTexture(GL_TEXTURE_2D, idColor);
			glTexImage2D(GL_TEXTURE_2D, 0, descriptor.typedFormat, (GLsizei)_width , (GLsizei)_height, 0, format, type, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)descriptor.filtering);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)descriptor.filtering);
		
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)descriptor.wrapping);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)descriptor.wrapping);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		
			if(descriptor.wrapping == GL_CLAMP_TO_BORDER){
				// Setup the border value for the shadow map
				GLfloat border[] = { 1.0, 1.0, 1.0, 1.0 };
				glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
			}
		
			// Link the texture to the color attachment (ie output) of the framebuffer.
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + GLuint(_idColors.size()), GL_TEXTURE_2D, idColor, 0);
			_idColors.push_back(idColor);
			_colorDescriptors.push_back(descriptor);
		}
	}
	
	if (!depthBufferSetup) {
		if (depthBuffer){
			// Create the renderbuffer (depth buffer).
			glGenRenderbuffers(1, &_idDepth);
			glBindRenderbuffer(GL_RENDERBUFFER, _idDepth);
			// Setup the depth buffer storage.
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, (GLsizei)_width, (GLsizei)_height);
			// Link the renderbuffer to the framebuffer.
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _idDepth);
			_depthUse = RENDERBUFFER;
		} else {
			_depthUse = NONE;
		}
	}
	
	//Register which color attachments to draw to.
	std::vector<GLenum> drawBuffers(_idColors.size());
	for(size_t i = 0; i < _idColors.size(); ++i){
		drawBuffers[i] = GL_COLOR_ATTACHMENT0 + GLuint(i);
	}
	glDrawBuffers(drawBuffers.size(), &drawBuffers[0]);
	checkGLFramebufferError();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	checkGLError();

}


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
	if (_depthUse == RENDERBUFFER) {
		glBindRenderbuffer(GL_RENDERBUFFER, _idDepth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, (GLsizei)_width, (GLsizei)_height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	} else if(_depthUse == TEXTURE){
		GLuint type, format;
		GLUtilities::getTypeAndFormat(_depthDescriptor.typedFormat, type, format);
		glBindTexture(GL_TEXTURE_2D, _idDepth);
		glTexImage2D(GL_TEXTURE_2D, 0, _depthDescriptor.typedFormat, (GLsizei)_width , (GLsizei)_height, 0, format, type, 0);
	}
	// Resize the textures.
	for(size_t i = 0; i < _idColors.size(); ++i){
		const auto & descriptor = _colorDescriptors[i];
		GLuint type, format;
		GLUtilities::getTypeAndFormat(descriptor.typedFormat, type, format);
		glBindTexture(GL_TEXTURE_2D, _idColors[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, descriptor.typedFormat, (GLsizei)_width, (GLsizei)_height, 0, format, type, 0);
	}
	
}

void Framebuffer::resize(glm::vec2 size){
	resize((unsigned int)size[0], (unsigned int)size[1]);
}

void Framebuffer::clean() const {
	if (_depthUse == RENDERBUFFER) {
		glDeleteRenderbuffers(1, &_idDepth);
	} else if(_depthUse == TEXTURE){
		glDeleteTextures(1, &_idDepth);
	}
	for(const auto idColor : _idColors){
		glDeleteTextures(1, &idColor);
	}
	glDeleteFramebuffers(1, &_id);
}



