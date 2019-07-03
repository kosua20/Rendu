#pragma once

#include "graphics/GLUtilities.hpp"
#include "Common.hpp"

/**
 \brief Represent a 2D rendering target, of any size, format and type, backed by an OpenGL framebuffer.
 \ingroup Graphics
 */
class Framebuffer {
	
public:
	
	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...).
	 Will use linear filtering and edge clamping.
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param typedFormat the precise typed format, combining format and type (GL_RGB8,...) to use
	 \param depthBuffer should the framebuffer contain a depth buffer to properly handle 3D geometry
	 */
	Framebuffer(unsigned int width, unsigned int height, const GLenum typedFormat, bool depthBuffer);
	
	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param descriptor the color attachment texture descriptor (format, filtering,...)
	 \param depthBuffer should the framebuffer contain a depth buffer to properly handle 3D geometry
	 */
	Framebuffer(unsigned int width, unsigned int height, const Descriptor & descriptor, bool depthBuffer);
	
	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param descriptors the color attachments texture descriptors (format, filtering,...)
	 \param depthBuffer should the framebuffer contain a depth buffer to properly handle 3D geometry
	 */
	Framebuffer(unsigned int width, unsigned int height, const std::vector<Descriptor> & descriptors, bool depthBuffer);
	
	/**
	 Bind the framebuffer.
	 */
	void bind() const;

	/**
	 Set the viewport to the size of the framebuffer.
	 */
	void setViewport() const;

	/**
	 Unbind the framebuffer.
	 \note Technically bind the window backbuffer.
	 */
	void unbind() const;
	
	/**
	 Resize the framebuffer to new dimensions.
	 \param width the new width
	 \param height the new height
	 */
	void resize(unsigned int width, unsigned int height);
	
	/**
	 Resize the framebuffer to new dimensions.
	 \param size the new size
	 */
	void resize(glm::vec2 size);
	
	/** Clean internal resources.
	 */
	void clean() const;
	
	/**
	 Query the ID of the 2D texture backing one of the color attachments.
	 \param i the color attachment index (or 0 by default)
	 \return the texture ID
	 */
	GLuint textureId(unsigned int i = 0) const { return _idColors[i]; }
	
	/**
	 Query the ID of the 2D textures backing all color attachments.
	 \return the texture IDs
	 */
	const std::vector<GLuint> textureIds() const { return _idColors; }
	
	/**
	 Query the ID of the 2D texture or renderbuffer backing the depth attachment.
	 \return the depth texture/renderbuffer ID
	 */
	GLuint depthId() const { return _depthUse == NONE ? 0 : _idDepth; }
	
	/**
	 Query the framebuffer width.
	 \return the width
	 */
	unsigned int width() const { return _width; }
	
	/**
	 Query the framebuffer height.
	 \return the height
	 */
	unsigned int height() const { return _height; }
	
	/**
	 Query the framebuffer ID.
	 \return the ID
	 */
	GLuint id() const { return _id; }
	
	/**
	 Query a color attachment OpenGL type and format.
	 \param i the color attachment index (or 0 by default)
	 \return the typed format
	 */
	GLuint typedFormat(unsigned int i = 0) const { return _colorDescriptors[i].typedFormat; }
	
private:
	
	unsigned int _width; ///< The framebuffer width.
	unsigned int _height; ///< The framebuffer height.
	
	GLuint _id; ///< The framebuffer ID.
	std::vector<GLuint> _idColors; ///< The color textures IDs.
	GLuint _idDepth; ///< The depth renderbuffer ID.
	
	std::vector<Descriptor> _colorDescriptors; ///< The color buffer descriptors.
	Descriptor _depthDescriptor; ///< The depth buffer descriptor.
	
	/// \brief Type of depth storage structure used.
	enum DepthBuffer {
		NONE, RENDERBUFFER, TEXTURE
	};
	DepthBuffer _depthUse; ///< The type of depth backing the framebuffer.
	
};
