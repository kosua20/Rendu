#pragma once

#include "resources/Texture.hpp"
#include "Common.hpp"

/**
 \brief Represent a rendering target, of any size, format and type, backed by a GPU framebuffer.
 \details Framebuffer can use different shapes: 2D, cubemap, 2D array, cubemap array, but you can only render to one 2D layer at a time.
 For cubemaps and arrays you can select the ayer when binding.
 \ingroup Graphics
 */
class Framebuffer {

public:
	/// \brief Framebuffer binding mode.
	enum class Mode {
		READ,  ///< Read mode.
		WRITE ///< Write mode.
	};

	enum class Operation : uint {
		LOAD,
		CLEAR,
		DONTCARE
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


	struct LoadOperation {

		LoadOperation() {};

		LoadOperation(Operation mod) : mode(mod) {};

		LoadOperation(const glm::vec4& val) : value(val), mode(Operation::CLEAR) {};

		LoadOperation(float val) : value(val), mode(Operation::CLEAR) {};

		LoadOperation(uchar val) : value(float(val)), mode(Operation::CLEAR) {};

		glm::vec4 value{1.0f};
		Operation mode = Operation::LOAD;
	};
	
	/**
	 Bind the framebuffer. Shortcut for writing to a 2D framebuffer.
	 */
	void bind(const LoadOperation& colorOp, const LoadOperation& depthOp = {}, const LoadOperation& stencilOp = {}) const;

	/**
	 Bind a specific layer of the framebuffer
	 \param layer the layer to bind
	 \param mip the mip level to bind
	 \param mode the mode to use
	 */
	void bind(size_t layer, size_t mip, const LoadOperation& colorOp, const LoadOperation& depthOp = {}, const LoadOperation& stencilOp = {}) const;

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
	 \param color clear color
	 \param depth clear depth
	 */
	void clear(const glm::vec4 & color, float depth);

	bool isEquivalent(const Framebuffer& other) const;

	VkRenderPass getRenderPass() const { return _renderPasses[0][0][0]; }

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
		// _colors will never be modified after initialization, so this can be done.
		return &_colors[i];
	}

	/**
	 Query the 2D texture backing one of the color attachments.
	 \param i the color attachment index (or 0 by default)
	 \return the texture
	 */
	Texture * texture(unsigned int i = 0) {
		// _colors will never be modified after initialization, so this can be done.
		return &_colors[i];
	}

	/** Query the descriptor of one of the color attachments.
	 \param i the color attachment index (or 0 by default)
	 \return the descriptor
	*/
	const Descriptor & descriptor(unsigned int i = 0) const {
		return _colors[i].gpu->descriptor();
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

	struct LayoutState {
		std::vector<Layout> colors;
		Layout depth;
		bool hasDepth = false;

		bool isEquivalent(const LayoutState& other) const;

	};

	const LayoutState& getLayoutState() const;

	/**
	 Query the window backbuffer infos.
	 \return a reference to a placeholder representing the backbuffer
	 \note Can be used in conjonction with saveFramebuffer() to save the content of the window.
	 */
	static Framebuffer * backbuffer();

	/**
	 Update the stored resolution of the backbuffer.
	 \param w the width
	 \param h the height
	 \warning This will not resize the backbuffer, only update the stored dimensions.
	 */

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

	struct Slice {
		VkFramebuffer framebuffer = VK_NULL_HANDLE; ///< The framebuffer ID.
		std::vector<VkImageView> attachments;
	};

	VkRenderPass createRenderpass(Operation colorOp, Operation depthOp, Operation stencilOp, bool presentable);

	void populateRenderPasses(bool isBackbuffer);
	
	void populateLayoutState();

	void finalizeFramebuffer();

	void bind(const Slice& slice, size_t layer, size_t mip, const LoadOperation& colorOp, const LoadOperation& depthOp, const LoadOperation& stencilOp) const;

	/** Default constructor. */
	Framebuffer() = default;

	std::string _name; ///< Framebuffer debug name.
	uint _width  = 0; ///< The framebuffer width.
	uint _height = 0; ///< The framebuffer height.
	uint _layers = 1; ///< The framebuffer depth.
	uint _mips = 1;

	std::vector<std::vector<Slice>> _framebuffers;

	std::vector<Texture> _colors; ///< The color textures.
	Texture _depth = Texture("Depth"); ///< The depth renderbuffer.

	TextureShape _shape = TextureShape::D2;	///< The texture shape.
	
	std::array<std::array<std::array<VkRenderPass, 3>, 3>, 3> _renderPasses;

	LayoutState _state;

	bool _hasDepth = false; ///< The type of depth backing the framebuffer.
	bool _isBackbuffer = false;

	static Framebuffer * _backbuffer; ///< Dummy backbuffer framebuffer.
	
	friend class GPU; ///< Utilities will need to access GPU handle.
	friend class Swapchain;
};
