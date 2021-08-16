#pragma once

#include "graphics/GPUTypes.hpp"
#include "resources/Texture.hpp"
#include "Common.hpp"

#include <array>

// Forward declarations.
VK_DEFINE_HANDLE(VkFramebuffer)
VK_DEFINE_HANDLE(VkImageView)
VK_DEFINE_HANDLE(VkRenderPass)

/**
 \brief Represent a rendering target, of any size, format and type, backed by a GPU framebuffer.
 \details Framebuffer can use different shapes: 2D, cubemap, 2D array, cubemap array, but you can only render to one 2D layer at a time.
 For cubemaps and arrays you can select the layer when binding. For mipmapped framebuffers, you also select the mip level.
 \ingroup Graphics
 */
class Framebuffer {

public:

	/** \brief Type of operation to perform when binding a framebuffer (starting a renderpass).
	 *  \sa LoadOperation
	 */
	enum class Operation : uint {
		LOAD, ///< Load existing data
		CLEAR, ///< Clear existing data
		DONTCARE ///< Anything can be done, usually because we will overwrite data everywhere.
	};

	/// \brief Detailed operation to perform when binding a framebuffer (starting a renderpass).
	struct LoadOperation {

		/// Default operation.
		LoadOperation() {};

		/** Specific operation
		 * \param mod the operation to perform
		 */
		LoadOperation(Operation mod) : mode(mod) {};

		/** Clear color operation.
		 * \param val the color to clear with
		 */
		LoadOperation(const glm::vec4& val) : value(val), mode(Operation::CLEAR) {};

		/** Clear depth operation.
		 * \param val the depth to clear with
		 */
		LoadOperation(float val) : value(val), mode(Operation::CLEAR) {};

		/** Clear stencil operation.
		 * \param val the stencil value to clear with
		 */
		LoadOperation(uchar val) : value(float(val)), mode(Operation::CLEAR) {};

		glm::vec4 value{1.0f}; ///< Clear value.
		Operation mode = Operation::LOAD; ///< Operation.
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
	 Bind the framebuffer, beginning a new renderpass. Shortcut for writing to a one-mip, one-layer 2D framebuffer.
	 \param colorOp the operation to perform on the color attachments
	 \param depthOp the operation to perform on the depth attachment
	 \param stencilOp the operation to perform on the stencil attachment
	 */
	void bind(const LoadOperation& colorOp, const LoadOperation& depthOp = {}, const LoadOperation& stencilOp = {}) const;

	/**
	 Bind a specific layer and level of the framebuffer, beginning a new renderpass.
	 \param layer the layer to bind
	 \param mip the mip level to bind
	 \param colorOp the operation to perform on the color attachments
	 \param depthOp the operation to perform on the depth attachment
	 \param stencilOp the operation to perform on the stencil attachment
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
	 \param color color to clear with
	 \param depth depth value to clear with
	 */
	void clear(const glm::vec4 & color, float depth);

	/** Check if another framebuffer is compatible with this one (ie if the formats of all attachments are equal).
	 * \param other the framebuffer to compare to
	 * \return true if compatible
	 */
	bool isEquivalent(const Framebuffer& other) const;

	/** \return a render pass compatible with this framebuffer */
	VkRenderPass getRenderPass() const { return _renderPasses[0][0][0]; }

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
	const Descriptor & descriptor(unsigned int i = 0) const;

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

	/// The framebuffer pipeline state.
	struct State {
		std::vector<Layout> colors; ///< Color attachment layouts.
		Layout depth; ///< Depth-stencil layout.
		bool hasDepth = false; ///< Does the framebuffer have a depth attachment.

		/** Check if another state is compatible. 
			\param other the state to compare to
			\return true if both states are equivalent for a pipeline
		 */
		bool isEquivalent(const State& other) const;

	};

	/// \return the framebuffer pipeline state
	const State& getState() const;

	/**
	 Query the window backbuffer.
	 \return a reference to a placeholder representing the backbuffer
	 \note Can be used in conjonction with saveFramebuffer() to save the content of the window.
	 */
	static Framebuffer * backbuffer();

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

	/// \brief View on one layer of one level of the framebuffer.
	struct Slice {
		VkFramebuffer framebuffer = VK_NULL_HANDLE; ///< Native handle.
		std::vector<VkImageView> attachments; ///< Views for all attachments.
	};

	/** Create a render pass for the corresponding load operations.
	 *  \param colorOp the operation to perform on the color attachments
	 *  \param depthOp the operation to perform on the depth attachment
	 *  \param stencilOp the operation to perform on the stencil attachment
	 *  \param presentable should the framebuffer end up in a presentable state (if its the backbuffer)
	 *  \return the newly created render pass
	 */
	VkRenderPass createRenderpass(Operation colorOp, Operation depthOp, Operation stencilOp, bool presentable);

	/** Create all possible render passes.
	 * \param isBackbuffer will the framebuffer be used as backbuffer (specific presentation end layout)
	 */
	void populateRenderPasses(bool isBackbuffer);
	
	/** Populate the framebuffer pipeline state. */
	void populateLayoutState();

	/** Create all required framebuffer views and structures.*/
	void finalizeFramebuffer();

	/** Default constructor. */
	Framebuffer() = default;

	std::vector<std::vector<Slice>> _framebuffers; ///< Per-level per-layer framebuffer info.
	std::vector<Texture> _colors; ///< The color textures.
	Texture _depth = Texture("Depth"); ///< The depth texture.
	
	std::array<std::array<std::array<VkRenderPass, 3>, 3>, 3> _renderPasses; ///< Render passes for all operations possible combinations.
	State _state; ///< The framebuffer pipeline state.

	glm::vec4 _readColor = glm::vec4(0.0f); ///< Buffered read-back pixel color.
	GPUAsyncTask _readTask = 0; ///< Read-back async task.

	std::string _name; ///< Framebuffer debug name.
	TextureShape _shape = TextureShape::D2;	///< The texture shape.
	uint _width  = 0; ///< The framebuffer width.
	uint _height = 0; ///< The framebuffer height.
	uint _layers = 1; ///< The framebuffer depth.
	uint _mips = 1; ///< The number of mip levels.
	bool _hasDepth = false; ///< Does the framebuffer have a depth-stencil attachment.
	bool _isBackbuffer = false; ///< Is the framebuffer used as backbuffer.

	static Framebuffer * _backbuffer; ///< Current backbuffer framebuffer.
	
	friend class GPU; ///< Utilities will need to access GPU handle.
	friend class Swapchain; ///< For backbuffer custom setup.
};
