#pragma once

#include "Common.hpp"
#include "graphics/GPUObjects.hpp"

#include <volk/volk.h>

#define FORCE_DEBUG_VULKAN

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
	uint32_t graphicsId = 0;
	uint32_t presentId = 0;
	uint32_t currentFrame = 0;

	double timestep = 0.0;
	size_t uniformAlignment = 0;
	bool portability = false;
	
	VkCommandBuffer& getCurrentCommandBuffer(){
		return commandBuffers[currentFrame];
	}

};



VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);

namespace VkUtils {

	bool checkLayersSupport(const std::vector<const char*> & requestedLayers);

	bool checkExtensionsSupport(const std::vector<const char*> & requestedExtensions);

	std::vector<const char*> getRequiredInstanceExtensions(const bool enableValidationLayers);

	bool checkDeviceExtensionsSupport(VkPhysicalDevice device, const std::vector<const char*> & requestedExtensions, bool & hasPortability);

	bool getQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, uint & graphicsFamily, uint & presentFamily);

	VkFormat findSupportedFormat(const VkPhysicalDevice & physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	void typesFromShape(const TextureShape & shape, VkImageType & imgType, VkImageViewType & viewType);

	uint32_t findMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags & properties, const VkPhysicalDevice & physicalDevice);

	VkCommandBuffer startOneTimeCommandBuffer(GPUContext & context);

	void endOneTimeCommandBuffer(VkCommandBuffer & commandBuffer, GPUContext & context);

	void transitionImageLayout(GPUContext & context, VkImage & image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, int mipCount, int layerCount);

	void createCommandBuffers(GPUContext & context, uint count);

}
