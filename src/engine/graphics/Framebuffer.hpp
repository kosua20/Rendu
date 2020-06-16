#pragma once

#include "resources/Texture.hpp"
#include "Common.hpp"

/**
 \brief Represent a rendering target, of any size, format and type, backed by an OpenGL framebuffer.
 \details Framebuffer can use different shapes: 2D, cubemap, 2D array, cubemap array, but you can only render to one 2D layer at a time.
 For cubemaps and arrays you can select the ayer when binding.
 \ingroup Graphics
 */
class Framebuffer {

public:
	/// \brief Framebuffer binding mode.
	enum class Mode {
		READ,  ///< Read mode.
		WRITE, ///< Write mode.
		SRGB   ///< Perform linear-to-sRGB conversion when writing/reading (if the framebuffer is backed by an sRGB texture).
	};

	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param descriptor the color attachment texture descriptor (format, filtering,...)
	 \param depthBuffer should the framebuffer contain a depth buffer to properly handle 3D geometry
	 \param name the framebuffer debug name
	 */
	Framebuffer(uint width, uint height, const Descriptor & descriptor, bool depthBuffer, const std::string & name);

	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param descriptors the color attachments texture descriptors (format, filtering,...)
	 \param depthBuffer should the framebuffer contain a depth buffer to properly handle 3D geometry
	 \param name the framebuffer debug name
	 */
	Framebuffer(uint width, uint height, const std::vector<Descriptor> & descriptors, bool depthBuffer, const std::string & name);

	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	 \param shape the texture shape (2D, cubemap, array,...)
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param depth the number of layers of the framebuffer
	 \param mips the number of mip levels of the framebuffer
	 \param descriptors the color attachments texture descriptors (format, filtering,...)
	 \param depthBuffer should the framebuffer contain a depth buffer to properly handle 3D geometry
	 \param name the framebuffer debug name
	 */
	Framebuffer(TextureShape shape, uint width, uint height, uint depth, uint mips, const std::vector<Descriptor> & descriptors, bool depthBuffer, const std::string & name);

	/**
	 Bind the framebuffer. Shortcut for writing to a 2D framebuffer.
	 */
	void bind() const;

	/**
	 Bind the framebuffer with specific options.
	 \param mode the mode to use
	 */
	void bind(Mode mode) const;

	/**
	 Bind a specific layer of the framebuffer
	 \param layer the layer to bind
	 \param mip the mip level to bind
	 \param mode the mode to use
	 */
	void bind(size_t layer, size_t mip = 0, Mode mode = Mode::WRITE) const;

	/**
	 Set the viewport to the size of the framebuffer.
	 */
	void setViewport() const;

	/**
	 Unbind the framebuffer.
	 \note Technically bind the window backbuffer with sRGB conversion disabled.
	 */
	void unbind() const;

	/**
	 Resize the framebuffer to new dimensions.
	 \param width the new width
	 \param height the new height
	 */
	void resize(uint width, uint height);

	/**
	 Resize the framebuffer to new dimensions.
	 \param size the new size
	 */
	void resize(const glm::ivec2 & size);

	/** Clear all levels of all layers of all attachments of the framebuffer.
	 \param color clear color
	 \param depth clear depth
	 */
	void clear(const glm::vec4 & color, float depth);

	/** Read back the value at a given pixel in the first color attachment.
	 \param pos the position in pixels
	 \return a float RGB color.
	 */
	glm::vec3 read(const glm::ivec2 & pos) const;

	/**
	 Query the 2D texture backing one of the color attachments.
	 \param i the color attachment index (or 0 by default)
	 \return the texture
	 */
	const Texture * texture(unsigned int i = 0) const {
		// _idColors will never be modified after initialization, so this can be done.
		return &_idColors[i];
	}

	/**
	 Query the 2D texture backing one of the color attachments.
	 \param i the color attachment index (or 0 by default)
	 \return the texture
	 */
	Texture * texture(unsigned int i = 0) {
		// _idColors will never be modified after initialization, so this can be done.
		return &_idColors[i];
	}

	/** Query the descriptor of one of the color attachments.
	 \param i the color attachment index (or 0 by default)
	 \return the descriptor
	*/
	const Descriptor & descriptor(unsigned int i = 0) const {
		return _idColors[i].gpu->descriptor();
	}

	/** Query the shape of the framebuffer.
	 \return the texture shape used for all attachments.
	 */
	const TextureShape & shape() const {
		return _shape;
	}

	/**
	 Query the 2D texture backing the depth attachment if it exists.
	 \return the depth texture or null
	 */
	const Texture * depthBuffer() const { return (_depthUse == Depth::TEXTURE ? &_idDepth : nullptr); }

	/** Query the name of the framebuffer, for debugging purposes.
	\return name the framebuffer name
	*/
	const std::string & name() const { return _name; }

	/**
	 Query the framebuffer width.
	 \return the width
	 */
	uint width() const { return _width; }

	/**
	 Query the framebuffer height.
	 \return the height
	 */
	uint height() const { return _height; }

	/**
	 Query the framebuffer depth.
	 \return the depth
	 */
	uint depth() const { return _depth; }

	/**
	 Query the number of color attachments.
	 \return the number of color attachments
	 */
	uint attachments() const;

	/**
	 Query the window backbuffer infos.
	 \return a reference to a placeholder representing the backbuffer
	 \note Can be used in conjonction with saveFramebuffer() to save the content of the window.
	 */
	static const Framebuffer * backbuffer();
	
	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Framebuffer & operator=(const Framebuffer &) = delete;
	
	/** Copy constructor (disabled). */
	Framebuffer(const Framebuffer &) = delete;
	
	/** Move assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Framebuffer & operator=(Framebuffer &&) = delete;
	
	/** Move constructor (disabled). */
	Framebuffer(Framebuffer &&) = delete;

	/** Destructor. */
	~Framebuffer();

private:
	/** Default constructor. */
	Framebuffer() = default;

	std::string _name; ///< Framebuffer debug name.
	unsigned int _width  = 0; ///< The framebuffer width.
	unsigned int _height = 0; ///< The framebuffer height.
	unsigned int _depth = 0; ///< The framebuffer depth.

	GLuint _id = 0;					///< The framebuffer ID.
	std::vector<Texture> _idColors; ///< The color textures.
	Texture _idDepth = Texture("Depth"); ///< The depth renderbuffer.
	TextureShape _shape = TextureShape::D2;	///< The texture shape.
	GLenum _target = GL_TEXTURE_2D;			///< The OpenGL texture shape.
	bool _hasStencil = false;		///< Does the framebuffer has a stencil buffer.
	
	/// \brief Type of depth storage structure used.
	enum class Depth {
		NONE,
		RENDERBUFFER,
		TEXTURE
	};
	Depth _depthUse = Depth::NONE; ///< The type of depth backing the framebuffer.

	static Framebuffer * _backbuffer; ///< Dummy backbuffer framebuffer.
};
