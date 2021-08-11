#include "graphics/GPUInternal.hpp"
#include "resources/Texture.hpp"

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

	// Messages to ignore.
	static const std::vector<int32_t> idsToIgnore = { 0x609a13b /* UNASSIGNED-CoreValidation-Shader-OutputNotConsumed */};
	if(std::find(idsToIgnore.begin(), idsToIgnore.end(), callbackData->messageIdNumber) != idsToIgnore.end()){
		return VK_FALSE;
	}

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
		// In debug, breakpoint for convenience, after the log.
#ifdef DEBUG
	#ifdef _WIN32
			__debugbreak();
	#elif defined(__x86_64__)
			__asm__ volatile("int $0x03");
	#endif
#endif

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

	static const std::unordered_map<TextureShape, VkImageType> imgTypes = {
		{TextureShape::D1, VK_IMAGE_TYPE_1D},
		{TextureShape::D2, VK_IMAGE_TYPE_2D},
		{TextureShape::D3, VK_IMAGE_TYPE_3D},

		{TextureShape::Cube, VK_IMAGE_TYPE_2D},
		{TextureShape::Array1D, VK_IMAGE_TYPE_1D},
		{TextureShape::Array2D, VK_IMAGE_TYPE_2D},
		{TextureShape::ArrayCube, VK_IMAGE_TYPE_2D}};

	imgType = imgTypes.at(shape);

	static const std::unordered_map<TextureShape, VkImageViewType> viewTypes = {
		{TextureShape::D1, VK_IMAGE_VIEW_TYPE_1D},
		{TextureShape::D2, VK_IMAGE_VIEW_TYPE_2D},
		{TextureShape::D3, VK_IMAGE_VIEW_TYPE_3D},
		{TextureShape::Cube, VK_IMAGE_VIEW_TYPE_CUBE},
		{TextureShape::Array1D, VK_IMAGE_VIEW_TYPE_1D_ARRAY},
		{TextureShape::Array2D, VK_IMAGE_VIEW_TYPE_2D_ARRAY},
		{TextureShape::ArrayCube, VK_IMAGE_VIEW_TYPE_CUBE_ARRAY}};

	viewType = viewTypes.at(shape);
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
	// \todo Instead of waiting for the queue to complete before submitting the next one, we could insert an event to
	// get a commandBuffer -> main command buffer dependency.
	// If we also insert a main command buffer -> command buffer dependency at the beginning, this will implicitely order auxiliary command buffers between them.
	// We could also submit to the main command buffer if it exists.
	vkQueueWaitIdle(context.graphicsQueue);
	vkFreeCommandBuffers(context.device, context.commandPool, 1, &commandBuffer);
	commandBuffer = VK_NULL_HANDLE;
}

void VkUtils::imageLayoutBarrier(VkCommandBuffer& commandBuffer, GPUTexture& texture, VkImageLayout newLayout, uint mipStart, uint mipCount, uint layerStart, uint layerCount){

	static const std::unordered_map<VkImageLayout, std::vector<VkImageLayout>> allowedTransitions = {
		{VK_IMAGE_LAYOUT_UNDEFINED, { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } },

		{VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR } },

		{VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR } },

		{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR } },

		{VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } },

		{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL } },

		{VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, {VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL}}

	};
	struct BarrierSetting {
		VkAccessFlags access;
		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;
	};
	
	static const std::unordered_map<VkImageLayout, BarrierSetting> settings = {
		{ VK_IMAGE_LAYOUT_UNDEFINED, { 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT } },
		{ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, { VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT } },
		{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT } },
		{ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL , {  (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT } },
		{ VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, { (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT), VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT } },
		{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, { VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT  | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT } },
		{ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, { 0,  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT } },
	};

#ifdef DEBUG
#define VALIDATE_TRANSITIONS
#endif
	
	VkPipelineStageFlags srcStage = 0;
	VkPipelineStageFlags dstStage = 0;

	std::vector<VkImageMemoryBarrier> barriers;
	for(uint mid = mipStart; mid < mipStart + mipCount; ++mid){
		for(uint lid = layerStart; lid < layerStart + layerCount; ++lid){
			VkImageLayout oldLayout = texture.layouts[mid][lid];
			if(oldLayout == newLayout){
				continue;
			}
#ifdef VALIDATE_TRANSITIONS
			const std::vector<VkImageLayout>& list = allowedTransitions.at(oldLayout);
			const size_t listCount = list.size();
			size_t did = 0;
			for(;did < listCount; ++did){
				if(list[did] == newLayout){
					break;
				}
			}
			if(did == listCount){
				Log::Error() << Log::GPU << "Unsupported transition." << std::endl;
				continue;
			}
#endif
			
			barriers.emplace_back();
			VkImageMemoryBarrier& barrier = barriers.back();
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // We don't change queue here.
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = texture.image;
			barrier.subresourceRange.baseMipLevel = mid;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = lid;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.aspectMask = texture.aspect;

			const BarrierSetting& srcSetting = settings.at(oldLayout);
			const BarrierSetting& dstSetting = settings.at(newLayout);
			barrier.srcAccessMask = srcSetting.access;
			barrier.dstAccessMask = dstSetting.access;
			
			srcStage |= srcSetting.srcStage;
			dstStage |= dstSetting.dstStage;

			texture.layouts[mid][lid] = newLayout;
		}
	}

	// Already in the right layout.
	if(barriers.empty()){
		return;
	}

	vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, barriers.size(), barriers.data() );

}

void VkUtils::textureLayoutBarrier(VkCommandBuffer& commandBuffer, const Texture& texture, VkImageLayout newLayout){
	const bool isCube = texture.shape & TextureShape::Cube;
	const bool isArray = texture.shape & TextureShape::Array;
	const uint layers = (isCube || isArray) ? texture.depth : 1;
	VkUtils::imageLayoutBarrier(commandBuffer, *texture.gpu, newLayout, 0, texture.levels, 0, layers);
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
