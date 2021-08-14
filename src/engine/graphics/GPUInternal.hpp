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

//#define FORCE_DEBUG_VULKAN

#define VK_RETURN_CHECK(F, L) do {\
VkResult res = (F);\
if(res != VK_SUCCESS){\
Log::Error() << Log::GPU << "Return error at line " << L << std::endl;\
}\
} while(0);

#define VK_RET(F) VK_RETURN_CHECK((F), __LINE__)

/**
 \ingroup Graphics
 */


struct ResourceToDelete {
	std::string name;
	VkImageView view = VK_NULL_HANDLE;
	VkSampler sampler = VK_NULL_HANDLE;
	VkImage image = VK_NULL_HANDLE;
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation data = VK_NULL_HANDLE;
	VkFramebuffer framebuffer = VK_NULL_HANDLE;
	VkRenderPass renderPass = VK_NULL_HANDLE;
	uint64_t frame = 0;
};

struct AsyncTextureTask {
	std::shared_ptr<TransferBuffer> dstBuffer;
	std::unique_ptr<Texture> dstTexture;
	std::function<void(const Texture&)> callback;
	glm::uvec2 dstImageRange{0.0f,0.0f};
	uint64_t frame = 0;
	GPUAsyncTask id = 0;
};

struct GPUContext {

	VkInstance instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkCommandPool commandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> commandBuffers;
	VkQueue graphicsQueue= VK_NULL_HANDLE;
	VkQueue presentQueue= VK_NULL_HANDLE;
	DescriptorAllocator descriptorAllocator;
	std::unordered_map<GPUQuery::Type, QueryAllocator> queryAllocators;
	PipelineCache pipelineCache;
	VkPipeline pipeline = VK_NULL_HANDLE;

	std::deque<ResourceToDelete> resourcesToDelete;
	std::deque<AsyncTextureTask> textureTasks;
	uint64_t tasksCount = 0;


	
	uint32_t graphicsId = 0;
	uint32_t presentId = 0;

	uint32_t swapIndex = 0;
	uint64_t frameIndex = 0;

	double timestep = 0.0;
	size_t uniformAlignment = 0;
	bool portability = false;
	const uint frameCount = 2;
	bool newRenderPass = true;
	bool inRenderPass = false;
	
	void nextFrame(){
		++frameIndex;
		swapIndex = (uint32_t)(frameIndex % (uint64_t)frameCount);
	}

	VkCommandBuffer& getCurrentCommandBuffer(){
		return commandBuffers[swapIndex];
	}

};

class Texture;

VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);

namespace VkUtils {

	bool checkLayersSupport(const std::vector<const char*> & requestedLayers);

	bool checkExtensionsSupport(const std::vector<const char*> & requestedExtensions);

	std::vector<const char*> getRequiredInstanceExtensions(const bool enableValidationLayers);

	bool checkDeviceExtensionsSupport(VkPhysicalDevice device, const std::vector<const char*> & requestedExtensions, bool & hasPortability);

	bool getQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, uint & graphicsFamily, uint & presentFamily);

	VkFormat findSupportedFormat(const VkPhysicalDevice & physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	void typesFromShape(const TextureShape & shape, VkImageType & imgType, VkImageViewType & viewType);

	VkCommandBuffer startOneTimeCommandBuffer(GPUContext & context);

	void endOneTimeCommandBuffer(VkCommandBuffer & commandBuffer, GPUContext & context);

	void imageLayoutBarrier(VkCommandBuffer& commandBuffer, GPUTexture& texture, VkImageLayout newLayout, uint mipStart, uint mipCount, uint layerStart, uint layerCount);

	void textureLayoutBarrier(VkCommandBuffer& commandBuffer, const Texture& texture, VkImageLayout newLayout);

	void createCommandBuffers(GPUContext & context, uint count);

	void checkResult(VkResult status);

	VkSamplerAddressMode getGPUWrapping(Wrap mode);

	void getGPUFilter(Filter filtering, VkFilter & imgFiltering, VkSamplerMipmapMode & mipFiltering);

	unsigned int getGPULayout(Layout typedFormat, VkFormat & format);



	/** Offsets and sizes are expressed at mip 0 in all cases */
	glm::uvec2 copyTextureRegionToBuffer(VkCommandBuffer& commandBuffer, const Texture & srcTexture, std::shared_ptr<TransferBuffer> & dstBuffer, uint mipStart, uint mipCount, uint layerStart, uint layerCount, const glm::uvec2& offset, const glm::uvec2& size);

	/** Offsets and sizes are expressed at mip 0 in all cases */
	void blitTexture(VkCommandBuffer& commandBuffer, const Texture& src, const Texture& dst, uint mipStartSrc, uint mipStartDst, uint mipCount, uint layerStartSrc, uint layerStartDst, uint layerCount, const glm::uvec2& srcBaseOffset, const glm::uvec2& srcBaseSize, const glm::uvec2& dstBaseOffset, const glm::uvec2& dstBaseSize, Filter filter);

}


/** Hash specialization for unordered_map/set */
template <> struct std::hash<VkImageLayout> {
	std::size_t operator()(const VkImageLayout& t) const { return static_cast<std::size_t>(t); }
};
