#pragma once

#include "graphics/GPUTypes.hpp"
#include "resources/Texture.hpp"
#include "Common.hpp"

#include <array>

/**
 \brief Represent a rendering target, of any size, format and type, backed by a GPU framebuffer.
 \details Framebuffer can use different shapes: 2D, cubemap, 2D array, cubemap array, but you can only render to one 2D layer at a time.
 For cubemaps and arrays you can select the layer when binding. For mipmapped framebuffers, you also select the mip level.
 \ingroup Graphics
 */
class Framebuffer {

public:


	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param format the color attachment texture format
	 \param name the framebuffer debug name
	 */
	Framebuffer(uint width, uint height, const Layout & format, const std::string & name);

	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param formats the color attachments texture formats
	 \param name the framebuffer debug name
	 */
	Framebuffer(uint width, uint height, const std::vector<Layout> & formats, const std::string & name);

	/** Setup the framebuffer (attachments, renderbuffer, depth buffer, textures IDs,...)
	 \param shape the texture shape (2D, cubemap, array,...)
	 \param width the width of the framebuffer
	 \param height the height of the framebuffer
	 \param depth the number of layers of the framebuffer
	 \param mips the number of mip levels of the framebuffer
	 \param formats the color attachments texture formats
	 \param name the framebuffer debug name
	 */
	Framebuffer(TextureShape shape, uint width, uint height, uint depth, uint mips, const std::vector<Layout> & formats, const std::string & name);


	/**
	 Bind the framebuffer, beginning a new renderpass. Shortcut for writing to a one-mip, one-layer 2D framebuffer.
	 \param colorOp the operation to perform on the color attachments
	 \param depthOp the operation to perform on the depth attachment
	 \param stencilOp the operation to perform on the stencil attachment
	 */
	void bind(const Load& colorOp, const Load& depthOp = {}, const Load& stencilOp = {}) const;

	/**
	 Bind a specific layer and level of the framebuffer, beginning a new renderpass.
	 \param layer the layer to bind
	 \param mip the mip level to bind
	 \param colorOp the operation to perform on the color attachments
	 \param depthOp the operation to perform on the depth attachment
	 \param stencilOp the operation to perform on the stencil attachment
	 */
	void bind(uint layer, uint mip, const Load& colorOp, const Load& depthOp = {}, const Load& stencilOp = {}) const;

	/**
	 Set the viewport to the size of the framebuffer.
	 */
	void setViewport() const;

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
	 \param color color to clear with
	 \param depth depth value to clear with
	 */
	void clear(const glm::vec4 & color, float depth);

	/** Read back the value at a given pixel in the first layer and first level of the first color attachment.
	 \param pos the position in pixels
	 \return a float RGBA color.
	 */
	glm::vec4 read(const glm::uvec2 & pos);

	/**
	 Query the 2D texture backing one of the color attachments.
	 \param i the color attachment index (or 0 by default)
	 \return the texture
	 */
	const Texture * texture(uint i = 0) const {
		// _colors will never be modified after initialization, so this can be done.
		return &_colors[i];
	}

	/**
	 Query the 2D texture backing one of the color attachments.
	 \param i the color attachment index (or 0 by default)
	 \return the texture
	 */
	Texture * texture(uint i = 0) {
		// _colors will never be modified after initialization, so this can be done.
		return &_colors[i];
	}

	/** Query the format of one of the color attachments.
	 \param i the color attachment index (or 0 by default)
	 \return the texture format
	*/
	const Layout & format(uint i = 0) const;

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
	const Texture * depthBuffer() const { return (_hasDepth ? &_depth : nullptr); }

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
	uint depth() const { return _layers; }

	/**
	 Query the number of color attachments.
	 \return the number of color attachments
	 */
	uint attachments() const;

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

	std::vector<Texture> _colors; ///< The color textures.
	Texture _depth = Texture("Depth"); ///< The depth texture.

	glm::vec4 _readColor = glm::vec4(0.0f); ///< Buffered read-back pixel color.
	GPUAsyncTask _readTask = 0; ///< Read-back async task.

	std::string _name; ///< Framebuffer debug name.
	TextureShape _shape = TextureShape::D2;	///< The texture shape.
	uint _width  = 0; ///< The framebuffer width.
	uint _height = 0; ///< The framebuffer height.
	uint _layers = 1; ///< The framebuffer depth.
	uint _mips = 1; ///< The number of mip levels.
	bool _hasDepth = false; ///< Does the framebuffer have a depth-stencil attachment.
	
	friend class GPU; ///< Utilities will need to access GPU handle.
	friend class Swapchain; ///< For backbuffer custom setup.
};
