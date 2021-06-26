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

bool VkUtils::checkDeviceExtensionsSupport(VkPhysicalDevice device, const std::vector<const char*> & requestedExtensions, bool& hasPortability) {
	// Get available device extensions.
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	// Check for portability support.
	hasPortability = false;
	for(const auto& extensionProperties: availableExtensions){
		if(strcmp("VK_KHR_portability_subset", extensionProperties.extensionName) == 0){
			hasPortability = true;
			break;
		}
	}

	// Check if the required device extensions are available.
	for(const auto& extensionName : requestedExtensions) {
		bool extensionFound = false;
		for(const auto& extensionProperties : availableExtensions){
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

VkCommandBuffer VkUtils::startOneTimeCommandBuffer(GPUContext & context){
	// Create short-lived command buffer.
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = context.commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(context.device, &allocInfo, &commandBuffer);

	// Record in it immediatly.
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void VkUtils::endOneTimeCommandBuffer(VkCommandBuffer & commandBuffer, GPUContext & context){
	vkEndCommandBuffer(commandBuffer);
	// Submit it.
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(context.graphicsQueue);
	vkFreeCommandBuffers(context.device, context.commandPool, 1, &commandBuffer);
	commandBuffer = VK_NULL_HANDLE;
}

void VkUtils::transitionImageLayout(GPUContext & context, VkImage & image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, int mipCount, int layerCount) {

	VkCommandBuffer commandBuffer = VkUtils::startOneTimeCommandBuffer(context);

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
	barrier.subresourceRange.layerCount = layerCount;
	// Aspect mask.
	if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		// Also stencil if this is a mixed format.
		if(format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT){
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	} else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	// Masks for triggers.
	if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
		barrier.srcAccessMask = 0; //  As soon as possible.
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Before transfer.
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // After a transfer.
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; // Before the shader.
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0; //  As soon as possible.
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; // Before using it.
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		Log::Error() << Log::GPU << "Unsupported layout transition." << std::endl;
		return;
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr,  0, nullptr,  1, &barrier );

	VkUtils::endOneTimeCommandBuffer(commandBuffer, context);
}

void VkUtils::createCommandBuffers(GPUContext & context, uint count){
	// See if we can move their creation and deletion outside of the swapchain
	context.commandBuffers.resize(count);
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = context.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(count);
	if(vkAllocateCommandBuffers(context.device, &allocInfo, context.commandBuffers.data()) != VK_SUCCESS) {
		Log::Error() << Log::GPU  << "Unable to create command buffers." << std::endl;
		return;
	}
}


void VkUtils::checkResult(VkResult status){
	std::string errorType;
	switch(status){
		case VK_NOT_READY : { errorType = "VK_NOT_READY"; break; }
		case VK_TIMEOUT : { errorType = "VK_TIMEOUT"; break; }
		case VK_EVENT_SET : { errorType = "VK_EVENT_SET"; break; }
		case VK_EVENT_RESET : { errorType = "VK_EVENT_RESET"; break; }
		case VK_INCOMPLETE : { errorType = "VK_INCOMPLETE"; break; }
		case VK_ERROR_OUT_OF_HOST_MEMORY : { errorType = "VK_ERROR_OUT_OF_HOST_MEMORY"; break; }
		case VK_ERROR_OUT_OF_DEVICE_MEMORY : { errorType = "VK_ERROR_OUT_OF_DEVICE_MEMORY"; break; }
		case VK_ERROR_INITIALIZATION_FAILED : { errorType = "VK_ERROR_INITIALIZATION_FAILED"; break; }
		case VK_ERROR_DEVICE_LOST : { errorType = "VK_ERROR_DEVICE_LOST"; break; }
		case VK_ERROR_MEMORY_MAP_FAILED : { errorType = "VK_ERROR_MEMORY_MAP_FAILED"; break; }
		case VK_ERROR_LAYER_NOT_PRESENT : { errorType = "VK_ERROR_LAYER_NOT_PRESENT"; break; }
		case VK_ERROR_EXTENSION_NOT_PRESENT : { errorType = "VK_ERROR_EXTENSION_NOT_PRESENT"; break; }
		case VK_ERROR_FEATURE_NOT_PRESENT : { errorType = "VK_ERROR_FEATURE_NOT_PRESENT"; break; }
		case VK_ERROR_INCOMPATIBLE_DRIVER : { errorType = "VK_ERROR_INCOMPATIBLE_DRIVER"; break; }
		case VK_ERROR_TOO_MANY_OBJECTS : { errorType = "VK_ERROR_TOO_MANY_OBJECTS"; break; }
		case VK_ERROR_FORMAT_NOT_SUPPORTED : { errorType = "VK_ERROR_FORMAT_NOT_SUPPORTED"; break; }
		case VK_ERROR_FRAGMENTED_POOL : { errorType = "VK_ERROR_FRAGMENTED_POOL"; break; }
		case VK_ERROR_UNKNOWN : { errorType = "VK_ERROR_UNKNOWN"; break; }
		case VK_ERROR_OUT_OF_POOL_MEMORY : { errorType = "VK_ERROR_OUT_OF_POOL_MEMORY"; break; }
		case VK_ERROR_INVALID_EXTERNAL_HANDLE : { errorType = "VK_ERROR_INVALID_EXTERNAL_HANDLE"; break; }
		case VK_ERROR_FRAGMENTATION : { errorType = "VK_ERROR_FRAGMENTATION"; break; }
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS : { errorType = "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS"; break; }
		case VK_ERROR_SURFACE_LOST_KHR : { errorType = "VK_ERROR_SURFACE_LOST_KHR"; break; }
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR : { errorType = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR"; break; }
		case VK_SUBOPTIMAL_KHR : { errorType = "VK_SUBOPTIMAL_KHR"; break; }
		case VK_ERROR_OUT_OF_DATE_KHR : { errorType = "VK_ERROR_OUT_OF_DATE_KHR"; break; }
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR : { errorType = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR"; break; }
		case VK_ERROR_VALIDATION_FAILED_EXT : { errorType = "VK_ERROR_VALIDATION_FAILED_EXT"; break; }
		case VK_ERROR_INVALID_SHADER_NV : { errorType = "VK_ERROR_INVALID_SHADER_NV"; break; }
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT : { errorType = "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT"; break; }
		case VK_ERROR_NOT_PERMITTED_EXT : { errorType = "VK_ERROR_NOT_PERMITTED_EXT"; break; }
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT : { errorType = "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT"; break; }
		case VK_THREAD_IDLE_KHR : { errorType = "VK_THREAD_IDLE_KHR"; break; }
		case VK_THREAD_DONE_KHR : { errorType = "VK_THREAD_DONE_KHR"; break; }
		case VK_OPERATION_DEFERRED_KHR : { errorType = "VK_OPERATION_DEFERRED_KHR"; break; }
		case VK_OPERATION_NOT_DEFERRED_KHR : { errorType = "VK_OPERATION_NOT_DEFERRED_KHR"; break; }
		case VK_PIPELINE_COMPILE_REQUIRED_EXT : { errorType = "VK_PIPELINE_COMPILE_REQUIRED_EXT"; break; }
		case VK_SUCCESS:
		default:
			return;
			break;
	}

	Log::Error() << "Vulkan error : " << errorType << std::endl;

}
