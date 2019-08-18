#pragma once

#include "resources/Texture.hpp"
#include "Common.hpp"

/**
 \brief Represent a 2D rendering target, of any size, format and type, backed by an OpenGL framebuffer.
 \ingroup Graphics
 */
class Framebuffer {
	
public:
	
	/// \brief Framebuffer binding mode.
	enum class Mode {
		READ, ///< Read mode.
		WRITE ///< Write mode.
	};
	
	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...).
	 Will use linear filtering and edge clamping.
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param typedFormat the precise typed format, combining format and type (RGB8,...) to use
	 \param depthBuffer should the framebuffer contain a depth buffer to properly handle 3D geometry
	 */
	Framebuffer(unsigned int width, unsigned int height, const Layout typedFormat, bool depthBuffer);
	
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
	 Bind the framebuffer in read or write mode.
	 \param mode the mode to use
	 */
	void bind(Mode mode) const;

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
	void clean();
	
	glm::vec3 read(const glm::ivec2 & pos) const;
	
	/**
	 Query the 2D texture backing one of the color attachments.
	 \param i the color attachment index (or 0 by default)
	 \return the texture
	 */
	const Texture * textureId(unsigned int i = 0) const {
		// _idColors will never be modified after initialization, so this can be done.
		return &_idColors[i];
	}
	
	/**
	 Query the 2D texture or renderbuffer backing the depth attachment.
	 \return the depth texture/renderbuffer
	 */
	const Texture * depthId() const { return (_depthUse == Depth::NONE ? nullptr : &_idDepth); }
	
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
	 Query the window backbuffer infos.
	 \return a reference to a placeholder representing the backbuffer
	 \note Can be used in conjonction with saveFramebuffer() to save the content of the window.
	 */
	static const Framebuffer & backbuffer();
	
private:
	
	/** Default constructor. */
	Framebuffer();
	
	unsigned int _width = 0; ///< The framebuffer width.
	unsigned int _height = 0; ///< The framebuffer height.
	
	GLuint _id = 0; ///< The framebuffer ID.
	std::vector<Texture> _idColors; ///< The color textures.
	Texture _idDepth; ///< The depth renderbuffer.
	
	/// \brief Type of depth storage structure used.
	enum class Depth {
		NONE, RENDERBUFFER, TEXTURE
	};
	Depth _depthUse = Depth::NONE; ///< The type of depth backing the framebuffer.
	
	static Framebuffer * defaultFramebuffer; ///< Dummy backbuffer framebuffer.
};
