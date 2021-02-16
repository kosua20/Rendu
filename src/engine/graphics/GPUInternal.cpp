#include "graphics/GPUInternal.hpp"

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#include <GLFW/glfw3.h>
#include <map>


bool VkUtils::checkLayersSupport(const std::vector<const char*> & requestedLayers){
	// Get available layers.
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	// Cross check with those we want.
	for(const char* layerName : requestedLayers){
		bool layerFound = false;
		for(const auto& layerProperties : availableLayers){
			if(strcmp(layerName, layerProperties.layerName) == 0){
				layerFound = true;
				break;
			}
		}
		if(!layerFound){
			return false;
		}
	}
	return true;
}

bool VkUtils::checkExtensionsSupport(const std::vector<const char*> & requestedExtensions){
	// Get available extensions.
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
	for(const char* extensionName : requestedExtensions){
		bool extensionFound = false;
		for(const auto& extensionProperties: availableExtensions){
			if(strcmp(extensionName, extensionProperties.extensionName) == 0){
				extensionFound = true;
				break;
			}
		}
		if(!extensionFound){
			return false;
		}
	}
	return true;
}

std::vector<const char*> VkUtils::getRequiredInstanceExtensions(const bool enableValidationLayers){
	// Default Vulkan has no notion of surface/window. GLFW provide an implementation of the corresponding KHR extensions.
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	// If the validation layers are enabled, add associated extensions.
	if(enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extensions;
}

bool VkUtils::checkDeviceExtensionsSupport(VkPhysicalDevice device, const std::vector<const char*> & requestedExtensions) {
	// Get available device extensions.
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	// Check if the required device extensions are available.
	for(const auto& extensionName : requestedExtensions) {
		bool extensionFound = false;
		for(const auto& extensionProperties: availableExtensions){
			if(strcmp(extensionName, extensionProperties.extensionName) == 0){
				extensionFound = true;
				break;
			}
		}
		if(!extensionFound){
			return false;
		}
	}
	return true;
}

bool VkUtils::getQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, uint & graphicsFamily, uint & presentFamily){
	// Get all queues.
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
	// Find queue with graphics or presentation support.
	int i = 0;
	for(const auto& queueFamily : queueFamilies){
		// Check if queue support graphics.
		if(queueFamily.queueCount == 0){
			++i;
			continue;
		}

		if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphicsFamily = i;
		}
		// CHeck if queue support presentation.
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if(presentSupport) {
			presentFamily = i;
		}
		// If we have found both queues, exit.
		if(graphicsFamily >= 0 && presentFamily >= 0){
			return true;
		}

		++i;
	}
	return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData){
	(void)userData;
	// Build message.
	std::string message = "";
	if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT){
		message = "Validation: ";
	} else if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT){
		message = "Performance: ";
	}
	message.append(callbackData->pMessage);

	if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT){
		Log::Error() << Log::GPU << message << std::endl;
	} else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT){
		Log::Warning() << Log::GPU << message << std::endl;
	} else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT){
		Log::Info() << Log::GPU << message << std::endl;
	} else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT){
		Log::Verbose() << Log::GPU << message << std::endl;
	}
	return VK_FALSE;
}

VkFormat VkUtils::findSupportedFormat(const VkPhysicalDevice & physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features){
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	return VK_FORMAT_UNDEFINED;
}
/*

void VulkanUtilities::transitionImageLayout(const VkDevice & device, const VkCommandPool & commandPool, const VkQueue & queue, VkImage & image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, const bool cube, const uint32_t & mipCount) {
	VkCommandBuffer commandBuffer = beginOneShotCommandBuffer(device, commandPool);

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // We don't change queue here.
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipCount;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = cube ? 6 : 1;
	// Aspect mask.
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	} else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	// Masks for triggers.
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0; //  As soon as possible.
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Before transfer.
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // After a transfer.
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; // Before the shader.
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0; //  As soon as possible.
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; // Before using it.
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		std::cerr << "Unsupported layout transition." << std::endl;
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr,  0, nullptr,  1, &barrier );

	endOneShotCommandBuffer(commandBuffer, device, commandPool, queue);
}

VkImageView VulkanUtilities::createImageView(const VkDevice & device, const VkImage & image, const VkFormat format, const VkImageAspectFlags aspectFlags, const bool cube, const uint32_t & mipCount) {
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = cube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipCount;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = cube ? 6 : 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		std::cerr << "Unable to create image view." << std::endl;
	}
	return imageView;
}
*/

void VkUtils::typesFromShape(const TextureShape & shape, VkImageType & imgType, VkImageViewType & viewType){
	imgType = VK_IMAGE_TYPE_2D;
	if(shape & TextureShape::D3){
		imgType = VK_IMAGE_TYPE_3D;
	} else if(shape & TextureShape::D1){
		imgType = VK_IMAGE_TYPE_1D;
	}

	const std::map<TextureShape, VkImageViewType> viewTypes = {
		{TextureShape::D1, VK_IMAGE_VIEW_TYPE_1D},
		{TextureShape::D2, VK_IMAGE_VIEW_TYPE_2D},
		{TextureShape::D3, VK_IMAGE_VIEW_TYPE_3D},
		{TextureShape::Cube, VK_IMAGE_VIEW_TYPE_CUBE},
		{TextureShape::Array1D, VK_IMAGE_VIEW_TYPE_1D_ARRAY},
		{TextureShape::Array2D, VK_IMAGE_VIEW_TYPE_2D_ARRAY},
		{TextureShape::ArrayCube, VK_IMAGE_VIEW_TYPE_CUBE_ARRAY}};

	viewType = viewTypes.at(shape);
}

uint32_t VkUtils::findMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags & properties, const VkPhysicalDevice & physicalDevice) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	Log::Error() << Log::GPU << "Unable to find proper memory." << std::endl;
	return 0;
}
