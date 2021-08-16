#include "graphics/GPUInternal.hpp"
#include "graphics/GPU.hpp"
#include "resources/Texture.hpp"

#include <GLFW/glfw3.h>
#include <map>

bool VkUtils::checkLayersSupport(const std::vector<const char*> & requestedLayers){
	// Get available layers.
	uint32_t layerCount;
	VK_RET(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));
	std::vector<VkLayerProperties> availableLayers(layerCount);
	VK_RET(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));
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
	VK_RET(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	VK_RET(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()));
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
	VK_RET(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr));
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	VK_RET(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()));

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
		VK_RET(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport));
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
	VK_RET(vkAllocateCommandBuffers(context.device, &allocInfo, &commandBuffer));

	// Record in it immediatly.
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_RET(vkBeginCommandBuffer(commandBuffer, &beginInfo));
	return commandBuffer;
}

void VkUtils::endOneTimeCommandBuffer(VkCommandBuffer & commandBuffer, GPUContext & context){
	VK_RET(vkEndCommandBuffer(commandBuffer));
	// Submit it.
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	VK_RET(vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
	// \todo Instead of waiting for the queue to complete before submitting the next one, we could insert an event to
	// get a commandBuffer -> main command buffer dependency.
	// If we also insert a main command buffer -> command buffer dependency at the beginning, this will implicitely order auxiliary command buffers between them.
	// We could also submit to the main command buffer if it exists.
	VK_RET(vkQueueWaitIdle(context.graphicsQueue));
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

	vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, barriers.size(), barriers.data());

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

	Log::Error() << "Vulkan error code: " << errorType << std::endl;

}

VkSamplerAddressMode VkUtils::getGPUWrapping(Wrap mode) {
	static const std::unordered_map<Wrap, VkSamplerAddressMode> wraps = {
		{Wrap::CLAMP, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE},
		{Wrap::REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT},
		{Wrap::MIRROR, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT}};
	return wraps.at(mode);
}

void VkUtils::getGPUFilter(Filter filtering, VkFilter & imgFiltering, VkSamplerMipmapMode & mipFiltering) {
	struct Filters {
		VkFilter img;
		VkSamplerMipmapMode mip;
	};
	static const std::unordered_map<Filter, Filters> filters = {
		{Filter::NEAREST, {VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST}},
		{Filter::LINEAR, {VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST}},
		{Filter::NEAREST_NEAREST, {VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST}},
		{Filter::LINEAR_NEAREST, {VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST}},
		{Filter::NEAREST_LINEAR, {VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_LINEAR}},
		{Filter::LINEAR_LINEAR, {VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR}}};
	const auto & infos = filters.at(filtering);
	imgFiltering = infos.img;
	mipFiltering = infos.mip;
}


unsigned int VkUtils::getGPULayout(Layout typedFormat, VkFormat & format) {

	struct FormatAndChannels {
		VkFormat format;
		int channels;
	};

	static const std::unordered_map<Layout, FormatAndChannels> formatInfos = {
		{Layout::R8, { VK_FORMAT_R8_UNORM, 1 }},
		{Layout::RG8, { VK_FORMAT_R8G8_UNORM, 2 }},
		{Layout::RGBA8, { VK_FORMAT_R8G8B8A8_UNORM, 4 }},
		{Layout::BGRA8, { VK_FORMAT_B8G8R8A8_UNORM, 4 }},
		{Layout::SRGB8_ALPHA8, { VK_FORMAT_R8G8B8A8_SRGB, 4 }},
		{Layout::SBGR8_ALPHA8, { VK_FORMAT_B8G8R8A8_SRGB, 4 }},
		{Layout::R16, { VK_FORMAT_R16_UNORM, 1 }},
		{Layout::RG16, { VK_FORMAT_R16G16_UNORM, 2 }},
		{Layout::RGBA16, { VK_FORMAT_R16G16B16A16_UNORM, 4 }},
		{Layout::R8_SNORM, { VK_FORMAT_R8_SNORM, 1 }},
		{Layout::RG8_SNORM, { VK_FORMAT_R8G8_SNORM, 2 }},
		{Layout::RGBA8_SNORM, { VK_FORMAT_R8G8B8A8_SNORM, 4 }},
		{Layout::R16_SNORM, { VK_FORMAT_R16_SNORM, 1 }},
		{Layout::RG16_SNORM, { VK_FORMAT_R16G16_SNORM, 2 }},
		{Layout::R16F, { VK_FORMAT_R16_SFLOAT, 1 }},
		{Layout::RG16F, { VK_FORMAT_R16G16_SFLOAT, 2 }},
		{Layout::RGBA16F, { VK_FORMAT_R16G16B16A16_SFLOAT, 4 }},
		{Layout::R32F, { VK_FORMAT_R32_SFLOAT, 1 }},
		{Layout::RG32F, { VK_FORMAT_R32G32_SFLOAT, 2 }},
		{Layout::RGBA32F, { VK_FORMAT_R32G32B32A32_SFLOAT, 4 }},
		{Layout::RGB5_A1, { VK_FORMAT_R5G5B5A1_UNORM_PACK16, 4 }},
		{Layout::A2_BGR10, { VK_FORMAT_A2B10G10R10_UNORM_PACK32, 4 }},
		{Layout::A2_RGB10, { VK_FORMAT_A2R10G10B10_UNORM_PACK32, 4 }},
		{Layout::DEPTH_COMPONENT16, { VK_FORMAT_D16_UNORM, 1 }},
		{Layout::DEPTH_COMPONENT32F, { VK_FORMAT_D32_SFLOAT, 1 }},
		{Layout::DEPTH24_STENCIL8, { VK_FORMAT_D24_UNORM_S8_UINT, 1 }},
		{Layout::DEPTH32F_STENCIL8, { VK_FORMAT_D32_SFLOAT_S8_UINT, 1 }},
		{Layout::R8UI, { VK_FORMAT_R8_UINT, 1 }},
		{Layout::R16I, { VK_FORMAT_R16_SINT, 1 }},
		{Layout::R16UI, { VK_FORMAT_R16_UINT, 1 }},
		{Layout::R32I, { VK_FORMAT_R32_SINT, 1 }},
		{Layout::R32UI, { VK_FORMAT_R32_UINT, 1 }},
		{Layout::RG8I, { VK_FORMAT_R8G8_SINT, 2 }},
		{Layout::RG8UI, { VK_FORMAT_R8G8_UINT, 2 }},
		{Layout::RG16I, { VK_FORMAT_R16G16_SINT, 2 }},
		{Layout::RG16UI, { VK_FORMAT_R16G16_UINT, 2 }},
		{Layout::RG32I, { VK_FORMAT_R32G32_SINT, 2 }},
		{Layout::RG32UI, { VK_FORMAT_R32G32_UINT, 2 }},
		{Layout::RGBA8I, { VK_FORMAT_R8G8B8A8_SINT, 4 }},
		{Layout::RGBA8UI, { VK_FORMAT_R8G8B8A8_UINT, 4 }},
		{Layout::RGBA16I, { VK_FORMAT_R16G16B16A16_SINT, 4 }},
		{Layout::RGBA16UI, { VK_FORMAT_R16G16B16A16_UINT, 4 }},
		{Layout::RGBA32I, { VK_FORMAT_R32G32B32A32_SINT, 4 }},
		{Layout::RGBA32UI, { VK_FORMAT_R32G32B32A32_UINT, 4 }}
	};

	if(formatInfos.count(typedFormat) > 0) {
		const auto & infos	= formatInfos.at(typedFormat);
		format = infos.format;
		return infos.channels;
	}

	Log::Error() << Log::GPU << "Unable to find type and format (typed format " << uint(typedFormat) << ")." << std::endl;
	return 0;
}

glm::uvec2 VkUtils::copyTextureRegionToBuffer(VkCommandBuffer& commandBuffer, const Texture & srcTexture, std::shared_ptr<TransferBuffer> & dstBuffer, uint mipStart, uint mipCount, uint layerStart, uint layerCount, const glm::uvec2& offset, const glm::uvec2& size){

	const Layout floatFormats[5] = {Layout(0), Layout::R32F, Layout::RG32F, Layout::RGBA32F /* no 3 channels format */, Layout::RGBA32F};

	const bool is3D = srcTexture.shape == TextureShape::D3;
	const uint layers = is3D ? 1u : std::min<uint>(srcTexture.depth, layerCount);
	const uint depth = is3D ? srcTexture.depth : 1;
	const uint channels = srcTexture.gpu->channels;

	const uint wBase = std::max<uint>(1u, size[0] >> mipStart);
	const uint hBase = std::max<uint>(1u, size[1] >> mipStart);
	const uint dBase = std::max<uint>(1u, depth >> mipStart);

	Texture transferTexture("tmpTexture");
	transferTexture.width = wBase;
	transferTexture.height = hBase;
	transferTexture.depth = srcTexture.shape == TextureShape::D3 ? dBase : layers;
	transferTexture.levels = mipCount;
	transferTexture.shape = srcTexture.shape;

	GPU::setupTexture(transferTexture, {floatFormats[channels], Filter::LINEAR, Wrap::CLAMP}, false);
	// Final usage of the transfer texture after the blit.
	transferTexture.gpu->defaultLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	// This will reset the input texture to its default state.
	const glm::uvec2 srcSize(srcTexture.width, srcTexture.height);
	VkUtils::blitTexture(commandBuffer, srcTexture, transferTexture, mipStart, 0, mipCount, layerStart, 0, layers,
					 offset, size, glm::uvec2(0), glm::uvec2(wBase, hBase), Filter::NEAREST);

	std::vector<VkBufferImageCopy> blitRegions(transferTexture.levels);
	size_t currentCount = 0;
	size_t currentSize = 0;

	for(uint mid = 0; mid < mipCount; ++mid){
		// Compute the size of an image.
		const uint w = std::max<uint>(1u, transferTexture.width >> mid);
		const uint h = std::max<uint>(1u, transferTexture.height >> mid);
		const uint d = transferTexture.shape == TextureShape::D3 ? std::max<uint>(1u, dBase >> mid) : 1;

		// Copy region for this mip level.

		blitRegions[mid].bufferOffset = currentSize;
		blitRegions[mid].bufferRowLength = 0; // Tightly packed.
		blitRegions[mid].bufferImageHeight = 0; // Tightly packed.
		blitRegions[mid].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegions[mid].imageSubresource.mipLevel = mid;
		blitRegions[mid].imageSubresource.baseArrayLayer = 0;
		blitRegions[mid].imageSubresource.layerCount = layers;
		blitRegions[mid].imageOffset = {0, 0, 0};
		blitRegions[mid].imageExtent = { (uint32_t)w, (uint32_t)h, (uint32_t)d};

		// Number of images.
		const uint imageSize = w * h * sizeof(float) * channels;
		const uint imageCount = is3D ? d : layers;

		currentSize += imageSize * imageCount;
		currentCount += imageCount;
	}

	dstBuffer.reset(new TransferBuffer(currentSize, BufferType::GPUTOCPU));

	// Copy from the intermediate texture.
	vkCmdCopyImageToBuffer(commandBuffer, transferTexture.gpu->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstBuffer->gpu->buffer, blitRegions.size(), blitRegions.data());

	// Compute the index of the first image we will fill.
	size_t firstImage = 0;
	for(uint lid = 0; lid < mipStart; ++lid){
		firstImage += is3D ? std::max<uint>(1u, depth >> lid) : layers;
	}

	return glm::uvec2(firstImage, currentCount);
}

void VkUtils::blitTexture(VkCommandBuffer& commandBuffer, const Texture& src, const Texture& dst, uint mipStartSrc, uint mipStartDst, uint mipCount, uint layerStartSrc, uint layerStartDst, uint layerCount, const glm::uvec2& srcBaseOffset, const glm::uvec2& srcBaseSize, const glm::uvec2& dstBaseOffset, const glm::uvec2& dstBaseSize, Filter filter){

	const uint srcLayers = src.shape != TextureShape::D3 ? src.depth : 1;
	const uint dstLayers = dst.shape != TextureShape::D3 ? dst.depth : 1;

	const uint mipEffectiveCount = std::min(std::min(src.levels, dst.levels), mipCount);
	const uint layerEffectiveCount = std::min(std::min(srcLayers, dstLayers), layerCount);
	const uint srcEffectiveWidth = std::min(srcBaseSize[0], src.width - srcBaseOffset[0]);
	const uint srcEffectiveHeight = std::min(srcBaseSize[1], src.width - srcBaseOffset[1]);
	const uint dstEffectiveWidth = std::min(dstBaseSize[0], dst.width - dstBaseOffset[0]);
	const uint dstEffectiveHeight = std::min(dstBaseSize[1], dst.width - dstBaseOffset[1]);

	VkUtils::imageLayoutBarrier(commandBuffer, *src.gpu, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mipStartSrc, mipEffectiveCount, layerStartSrc, layerEffectiveCount);
	VkUtils::imageLayoutBarrier(commandBuffer, *dst.gpu, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipStartDst, mipEffectiveCount, layerStartDst, layerEffectiveCount);

	std::vector<VkImageBlit> blitRegions(mipEffectiveCount);

	const uint srcBaseDepth = src.shape == TextureShape::D3 ? src.depth : 1;
	const uint dstBaseDepth = dst.shape == TextureShape::D3 ? dst.depth : 1;

	const VkFilter filterVk = filter == Filter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;

	for(size_t mid = 0; mid < mipEffectiveCount; ++mid) {
		const uint srcMip = mipStartSrc + mid;
		const uint dstMip = mipStartDst + mid;

		const uint srcWidth  = std::max(srcEffectiveWidth >> srcMip, uint(1));
		const uint srcHeight = std::max(srcEffectiveHeight >> srcMip, uint(1));
		const uint srcDepth  = std::max(srcBaseDepth >> srcMip, uint(1));
		const glm::uvec2 srcOffset = srcBaseOffset >> glm::uvec2(srcMip);

		const uint dstWidth  = std::max(dstEffectiveWidth >> dstMip, uint(1));
		const uint dstHeight = std::max(dstEffectiveHeight >> dstMip, uint(1));
		const uint dstDepth  = std::max(dstBaseDepth >> dstMip, uint(1));
		const glm::uvec2 dstOffset = dstBaseOffset >> glm::uvec2(dstMip);

		blitRegions[mid].srcOffsets[0] = { int32_t(srcOffset[0]), int32_t(srcOffset[1]), 0};
		blitRegions[mid].dstOffsets[0] = { int32_t(dstOffset[0]), int32_t(dstOffset[1]), 0};
		blitRegions[mid].srcOffsets[1] = { int32_t(srcOffset[0] + srcWidth), int32_t(srcOffset[1] + srcHeight), int32_t(0 + srcDepth)};
		blitRegions[mid].dstOffsets[1] = { int32_t(dstOffset[0] + dstWidth), int32_t(dstOffset[1] + dstHeight), int32_t(0 + dstDepth)};
		blitRegions[mid].srcSubresource.aspectMask = src.gpu->aspect;
		blitRegions[mid].dstSubresource.aspectMask = dst.gpu->aspect;
		blitRegions[mid].srcSubresource.mipLevel = srcMip;
		blitRegions[mid].dstSubresource.mipLevel = dstMip;
		blitRegions[mid].srcSubresource.baseArrayLayer = layerStartSrc;
		blitRegions[mid].dstSubresource.baseArrayLayer = layerStartDst;
		blitRegions[mid].srcSubresource.layerCount = layerEffectiveCount;
		blitRegions[mid].dstSubresource.layerCount = layerEffectiveCount;
	}


	vkCmdBlitImage(commandBuffer, src.gpu->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst.gpu->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, blitRegions.size(), blitRegions.data(), filterVk);

	VkUtils::imageLayoutBarrier(commandBuffer, *src.gpu, src.gpu->defaultLayout, mipStartSrc, mipEffectiveCount, layerStartSrc, layerEffectiveCount);
	VkUtils::imageLayoutBarrier(commandBuffer, *dst.gpu, dst.gpu->defaultLayout, mipStartDst, mipEffectiveCount, layerStartDst, layerEffectiveCount);
}
