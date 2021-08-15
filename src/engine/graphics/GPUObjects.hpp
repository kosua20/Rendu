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

	VkFormat format; ///< Texture native format.
	VkSamplerAddressMode wrapping; ///< Sampler native wrapping.
	VkFilter imgFiltering; ///< Sampler native filtering.
	VkSamplerMipmapMode mipFiltering; ///< Sampler native mip filtering.
	VkImageAspectFlags aspect; ///< Texture aspects.
	
	uint channels; ///< Number of channels.

	VkImage image = VK_NULL_HANDLE; ///< Native image handle.
	VkImageView view = VK_NULL_HANDLE; ///< Native main image view (all mips).
	std::vector<VkImageView> levelViews;  ///< Per-mip image views.

	VmaAllocation data = VK_NULL_HANDLE; ///< Internal allocation.
	VkSampler sampler = VK_NULL_HANDLE; ///< Native sampler handle.
	ImTextureID imgui = (ImTextureID)VK_NULL_HANDLE; ///< ImGui compatible handle (internally a descriptor set).

	std::vector<std::vector<VkImageLayout>> layouts; ///< Per-mip per-layer image layout.
	VkImageLayout defaultLayout; ///< Default layout to restore to in some cases.

	std::string name; ///< Debug name. \todo Cleanup
	bool owned = true; ///< Do we own our Vulkan data (not the case for swapchain images).

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

	VkBuffer buffer = VK_NULL_HANDLE; ///< Buffer native handle.
	VmaAllocation data = VK_NULL_HANDLE; ///< Internal allocation.
	char* mapped = nullptr; ///< If the buffer is CPU-mappable, its CPU address.
	bool mappable = false; ///< Is the buffer mappable on the CPU.
	
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

	/** Test if two meshes have the same input configuration (attributes,...) 
	 * \param other the mesh to compare to
	 * \return true if the two are equivalent for a pipeline
	 */
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

	/** \brief Internal GPU state for pipeline compatibility. */
	struct State {
		std::vector<VkVertexInputAttributeDescription> attributes; ///< List of attributes.
		std::vector<VkVertexInputBindingDescription> bindings; ///< List of bindings.
		std::vector<VkBuffer> buffers; ///< Buffers used.
		std::vector<VkDeviceSize> offsets; ///< Offsets in each buffer.

		/** Check if another mesh state is compatible with this one.
		 * \param other the state to compare to
		 * \return true if equivalent
		 */
		bool isEquivalent(const State& other) const;
	};

	State state; ///< Internal GPU layout.
};
