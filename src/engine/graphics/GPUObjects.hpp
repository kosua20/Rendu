#pragma once

#include "Common.hpp"
#include "graphics/GPUTypes.hpp"

#include <volk/volk.h>

// Forward declaration.
VK_DEFINE_HANDLE(VmaAllocation);

/**
 \brief Store a texture data on the GPU.
 \ingroup Graphics
 */
class GPUTexture {
public:
	/** Constructor from a layout description and a texture shape.
	 \param texDescriptor the layout descriptor
	 \param shape the texture dimensionality
	 */
	GPUTexture(const Descriptor & texDescriptor);

	/** Clean internal GPU buffer. */
	void clean();

	/** Compare the texture layout to another one.
	 \param other the descriptor to compare to
	 \return true if the texture has a similar descriptor
	 */
	bool hasSameLayoutAs(const Descriptor & other) const;

	/** Set the texture filtering.
	 \param filtering the new filtering mode
	 */
	void setFiltering(Filter filtering);

	/** Query the texture descriptor.
	 \return the descriptor used
	 */
	const Descriptor & descriptor() const { return _descriptor; }
	
	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	GPUTexture & operator=(const GPUTexture &) = delete;
	
	/** Copy constructor (disabled). */
	GPUTexture(const GPUTexture &) = delete;
	
	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	GPUTexture & operator=(GPUTexture &&) = delete;
	
	/** Move constructor. */
	GPUTexture(GPUTexture &&) = delete;

	VkFormat format;
	VkSamplerAddressMode wrapping;
	VkFilter imgFiltering;
	VkSamplerMipmapMode mipFiltering;
	VkImageAspectFlags aspect;
	
	uint channels; ///< Number of channels.

	VkImage image = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	std::vector<VkImageView> levelViews;

	VmaAllocation data = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;
	ImTextureID imgui = (ImTextureID)VK_NULL_HANDLE;

	std::vector<std::vector<VkImageLayout>> layouts;
	VkImageLayout defaultLayout;

	std::string name;
	bool owned = true; //< Do we own our Vulkan data.
private:
	Descriptor _descriptor; ///< Layout used.
};


/**
 \brief Store data in a GPU buffer.
 \ingroup Graphics
 */
class GPUBuffer {
public:

	/** Constructor.
	 \param atype the type of buffer
	 \param use the update frequency
	 */
	GPUBuffer(BufferType atype);

	/** Clean internal GPU buffer. */
	void clean();

	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	GPUBuffer & operator=(const GPUBuffer &) = delete;

	/** Copy constructor (disabled). */
	GPUBuffer(const GPUBuffer &) = delete;

	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	GPUBuffer & operator=(GPUBuffer &&) = delete;

	/** Move constructor. */
	GPUBuffer(GPUBuffer &&) = delete;

	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation data = VK_NULL_HANDLE;
	char* mapped = nullptr;
	bool mappable = false;
	
};


/**
 \brief Store vertices and triangles data on the GPU, linked by an array object.
 \ingroup Graphics
 */
class GPUMesh {
public:
	
	std::unique_ptr<GPUBuffer> vertexBuffer; ///< Vertex data buffer.
	std::unique_ptr<GPUBuffer> indexBuffer; ///< Index element buffer.

	size_t count = 0; ///< The number of vertices (cached).
	
	/** Clean internal GPU buffers. */
	void clean();

	bool isEquivalent(const GPUMesh& other) const;
	
	/** Constructor. */
	GPUMesh() = default;
	
	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	GPUMesh & operator=(const GPUMesh &) = delete;
	
	/** Copy constructor (disabled). */
	GPUMesh(const GPUMesh &) = delete;
	
	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	GPUMesh & operator=(GPUMesh &&) = delete;
	
	/** Move constructor. */
	GPUMesh(GPUMesh &&) = delete;

	struct State {
		std::vector<VkVertexInputAttributeDescription> attributes;
		std::vector<VkVertexInputBindingDescription> bindings;
		std::vector<VkBuffer> buffers;
		std::vector<VkDeviceSize> offsets;

		bool isEquivalent(const State& other) const;
	};

	State state;
};
