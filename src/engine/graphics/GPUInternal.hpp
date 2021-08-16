#pragma once

#include "Common.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/DescriptorAllocator.hpp"
#include "graphics/QueryAllocator.hpp"
#include "graphics/PipelineCache.hpp"
#include "resources/Buffer.hpp"
#include <volk/volk.h>
#include <deque>
#include <functional>


// Forward declaration.
class Texture;

/**
 \addtogroup Graphics
 @{
 */

#ifdef DEBUG
/// Macro used to check the return code of Vulkan calls.
	#define VK_RETURN_CHECK(F, L) do {\
		VkResult res = (F);\
		if(res != VK_SUCCESS){\
			Log::Error() << Log::GPU << "Vulkan return error at line " << L << std::endl;\
			VkUtils::checkResult(res);\
		}\
	} while(0);
#else
	/// Macro used to check the return code of Vulkan calls.
	#define VK_RETURN_CHECK(F, L)
#endif

/// Macro used to check the return code of Vulkan calls.
#define VK_RET(F) VK_RETURN_CHECK((F), __LINE__)

/** @} */

/** \brief Request for the buffered deletion of a resource.
 * Any of the handles stored below is optional, depending on the kind of resource being deleted.
 \ingroup Graphics
 */
struct ResourceToDelete {
	std::string name; ///< Debug name.
	VkImageView view = VK_NULL_HANDLE; ///< Image view to delete.
	VkSampler sampler = VK_NULL_HANDLE; ///< Sampler to delete.
	VkImage image = VK_NULL_HANDLE; ///< Image to delete.
	VkBuffer buffer = VK_NULL_HANDLE; ///< Buffer to delete.
	VkFramebuffer framebuffer = VK_NULL_HANDLE; ///< Framebuffer to delete.
	VkRenderPass renderPass = VK_NULL_HANDLE; ///< Renderpass to delete.
	VmaAllocation data = VK_NULL_HANDLE; ///< Internal allocation to free.
	uint64_t frame = 0; ///< Frame where the deletion was requested.
};

/** \brief Request for an asynchronous texture download.
 * Data has been copied from the source texture to a buffer during the frame. The readback of the buffer to the CPU
 * has to be performed once the frame is complete. 
 \ingroup Graphics
 */
struct AsyncTextureTask {
	std::shared_ptr<TransferBuffer> dstBuffer; ///< Transfer buffer containing the texture data (mappable).
	std::unique_ptr<Texture> dstTexture; ///< Destination texture whose images have to be populated.
	std::function<void(const Texture&)> callback; ///< Callback to execute once the images creation is complete.
	glm::uvec2 dstImageRange{0u,0u}; ///< First index and number of images to populate (some mips/layers can be skipped). 
	uint64_t frame = 0; ///< Frame where the download was requested.
	GPUAsyncTask id = 0; ///< Async task ID.
};

/** \brief Global GPU context, storing all structures used for resource allocation and tracking, operations recording and rendering.
 \ingroup Graphics
 */
struct GPUContext {

	VkInstance instance = VK_NULL_HANDLE; ///< Native instance handle.
	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE; ///< Native debug extension handle.
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; ///< Native physical device handle.
	VkDevice device = VK_NULL_HANDLE; ///< Native logical device handle.
	VkSurfaceKHR surface = VK_NULL_HANDLE; ///< Native surface handle.
	VkCommandPool commandPool = VK_NULL_HANDLE; ///< Command pool for all frames.
	std::vector<VkCommandBuffer> commandBuffers; ///< Per-frame command buffers.
	VkQueue graphicsQueue= VK_NULL_HANDLE; ///< Graphics submission queue.
	VkQueue presentQueue= VK_NULL_HANDLE; ///< Presentation submission queue.
	DescriptorAllocator descriptorAllocator; ///< Descriptor sets common allocator.
	std::unordered_map<GPUQuery::Type, QueryAllocator> queryAllocators; ///< Per-type query buffered allocators.
	PipelineCache pipelineCache; ///< Pipeline cache and creation.

	std::deque<ResourceToDelete> resourcesToDelete; ///< List of resources waiting for deletion.
	std::deque<AsyncTextureTask> textureTasks; ///< List of async tasks waiting for completion.
	uint64_t tasksCount = 0; ///< Number of async tasks created.
	
	VkPipeline pipeline = VK_NULL_HANDLE; ///< Current pipeline.

	uint32_t graphicsId = 0; ///< Graphics queue index.
	uint32_t presentId = 0; ///< Present queue index.

	uint64_t frameIndex = 0; ///< Current frame index.
	uint32_t swapIndex = 0; ///< Current buffered frame (in 0, frameCount-1).

	double timestep = 0.0; ///< Query timing timestep.
	size_t uniformAlignment = 0; ///< Minimal buffer alignment.
	bool portability = false; ///< \todo Check.
	const uint frameCount = 2; ///< Number of buffered frames (should be lower or equal to the swapchain image count).
	bool newRenderPass = true; ///< Has a render pass just started (pipeline needs to be re-bound).
	bool inRenderPass = false; ///< Are we currently in a render pass. 
	
	/// Move to the next frame.
	void nextFrame(){
		++frameIndex;
		swapIndex = (uint32_t)(frameIndex % (uint64_t)frameCount);
	}

	/// \return the command buffer for the current frame.
	VkCommandBuffer& getCurrentCommandBuffer(){
		return commandBuffers[swapIndex];
	}

};

/** Callback for validation errors and warnings.
 * \param messageSeverity severity of message (error, warning,...)
 * \param messageType type of message (validation, performance,...)
 * \param callbackData message data, including text and ID.
 * \param userData custom user pointer
 * \return true if the callback completed successfully
 * \ingroup Graphics
 */
VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);

/** \brief Utility functions for the Vulkan backend.
 \ingroup Graphics
 */
namespace VkUtils {

	/** Check if the Vulkan instance supports some validation layers.
	 * \param requestedLayers list of layer names
	 * \return true if all layers are supported
	 */
	bool checkLayersSupport(const std::vector<const char*> & requestedLayers);

	/** Check if the Vulkan instance supports some extensions.
	 * \param requestedExtensions list of extension names
	 * \return true if all extensions are supported
	 */
	bool checkExtensionsSupport(const std::vector<const char*> & requestedExtensions);

	/** Query the instance extensions required by the system (swapchain, debug layers,...)
	 * \param enableValidationLayers should validation layer extensions be included 
	 * \return a list of extension names
	 */
	std::vector<const char*> getRequiredInstanceExtensions(const bool enableValidationLayers);

	/** Check if the Vulkan device supports some extensions and/or portability.
	 * \param device the physical device handle
	 * \param requestedExtensions list of extension names
	 * \param hasPortability will denote if the device support portability
	 * \return true if all extensions are supported
	 */
	bool checkDeviceExtensionsSupport(VkPhysicalDevice device, const std::vector<const char*> & requestedExtensions, bool & hasPortability);

	/** Query the graphics and present queue families indices.
	 * \param device the physical device handle
	 * \param surface the display surface
	 * \param graphicsFamily will contain the graphics queue family index
	 * \param presentFamily will contain the present queue family index
	 * \return true if query successful
	 */
	bool getQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, uint & graphicsFamily, uint & presentFamily);

	/** Find the optimal image format from a list based on tiling and feature constraints
	 * \param physicalDevice the physical device handle
	 * \param candidates list of possible formats to select from
	 * \param tiling type of tiling to support
	 * \param features feature flags to support
	 * \return the selected format
	 */
	VkFormat findSupportedFormat(const VkPhysicalDevice & physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	/** Convert a Rendu texture shape to an image and image view types.
	 * \param shape the texture shape
	 * \param imgType will contain the corresponding image type
	 * \param viewType will contain the corresponding image view type
	 */
	void typesFromShape(const TextureShape & shape, VkImageType & imgType, VkImageViewType & viewType);

	/** Start a one-shot command buffer.
	 * \param context the GPU internal context
	 * \return the newly created command buffer
	 */
	VkCommandBuffer startOneTimeCommandBuffer(GPUContext & context);

	/** Finish and submit a one-shot command buffer, waiting for queue completion.
	 * \param commandBuffer the command buffer to complete
	 * \param context the GPU internal context
	 */
	void endOneTimeCommandBuffer(VkCommandBuffer & commandBuffer, GPUContext & context);

	/** Apply a layout transition barrier to a texture region.
	 * \param commandBuffer the command buffer to record the operation on
	 * \param texture the texture to transition
	 * \param newLayout the new layout to apply
	 * \param mipStart first mip to apply the transition to
	 * \param mipCount number of mip levels to transition
	 * \param layerStart first layer to apply the transition to
	 * \param layerCount number of layers to transition
	 */
	void imageLayoutBarrier(VkCommandBuffer& commandBuffer, GPUTexture& texture, VkImageLayout newLayout, uint mipStart, uint mipCount, uint layerStart, uint layerCount);

	/** Apply a layout transition barrier to a whole texture.
	 * \param commandBuffer the command buffer to record the operation on
	 * \param texture the texture to transition
	 * \param newLayout the new layout to apply
	 */
	void textureLayoutBarrier(VkCommandBuffer& commandBuffer, const Texture& texture, VkImageLayout newLayout);

	/** Create per-frame command buffers on the context.
	 * \param context the GPU internal context
	 * \param count number of command buffers to create
	 */
	void createCommandBuffers(GPUContext & context, uint count);

	/** Check a Vulkan result (used by ImGui)
	 * \param status the status to check
	 * \todo cleanup
	 */
	void checkResult(VkResult status);

	/** Convert a Rendu texture wrapping to a Vulkan address mode.
	 * \param mode the texture wrapping
	 * \return the corresponding Vulkan adressing mode
	 */
	VkSamplerAddressMode getGPUWrapping(Wrap mode);

	/** Convert a Rendu texture filter to an interpolation and mipmap Vulkan filterings.
	 * \param filtering the texture filtering
	 * \param imgFiltering will contain the corresponding Vulkan image filtering
	 * \param mipFiltering will contain the corresponding Vulkan mipmap mode
	 */
	void getGPUFilter(Filter filtering, VkFilter & imgFiltering, VkSamplerMipmapMode & mipFiltering);

	/** Convert a Rendu texture layout to a format and channel count.
	 * \param typedFormat the texture layout
	 * \param format will contain the corresponding Vulkan format
	 * \return the format number of components
	 */
	unsigned int getGPULayout(Layout typedFormat, VkFormat & format);

	/** Copy a texture region to a transfer buffer allocated internally.
	 * \param commandBuffer the command buffer to record the operation on
	 * \param srcTexture the source texture
	 * \param dstBuffer will point to the destination buffer
	 * \param mipStart first mip to copy
	 * \param mipCount number of mip levels to copy
	 * \param layerStart first layer to copy
	 * \param layerCount number of layers to copy
	 * \param offset top-left corner pixel coordinates of the region to copy
	 * \param size pixel dimensions of the region to copy
	 * \note Offsets and sizes are expressed at mip 0 in all cases.
	 * \return the image index range in the source texture corresponding to the requested mip levels and layers.
	 */
	glm::uvec2 copyTextureRegionToBuffer(VkCommandBuffer& commandBuffer, const Texture & srcTexture, std::shared_ptr<TransferBuffer> & dstBuffer, uint mipStart, uint mipCount, uint layerStart, uint layerCount, const glm::uvec2& offset, const glm::uvec2& size);

	/** Blit a texture region to another texture region.
	 * \param commandBuffer the command buffer to record the operation on
	 * \param src the source texture
	 * \param dst the destination texture
	 * \param mipStartSrc first mip to copy from
	 * \param mipStartDst first mip to copy to
	 * \param mipCount number of mip levels to copy
	 * \param layerStartSrc first layer to copy from
	 * \param layerStartDst first layer to copy to
	 * \param layerCount number of layers to copy
	 * \param srcBaseOffset top-left corner pixel coordinates of the region to copy from
	 * \param srcBaseSize pixel dimensions of the region to copy from
	 * \param dstBaseOffset top-left corner pixel coordinates of the region to copy to
	 * \param dstBaseSize pixel dimensions of the region to copy to
	 * \param filter interpolation filtering to apply if regions have different dimensions
	 * \note Offsets and sizes are expressed at mip 0 in all cases.
	 */
	void blitTexture(VkCommandBuffer& commandBuffer, const Texture& src, const Texture& dst, uint mipStartSrc, uint mipStartDst, uint mipCount, uint layerStartSrc, uint layerStartDst, uint layerCount, const glm::uvec2& srcBaseOffset, const glm::uvec2& srcBaseSize, const glm::uvec2& dstBaseOffset, const glm::uvec2& dstBaseSize, Filter filter);

}

STD_HASH(VkImageLayout);
