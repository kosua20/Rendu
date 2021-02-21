#include "graphics/Swapchain.hpp"
#include "graphics/GPU.hpp"


Swapchain::Swapchain(){
	
}


void Swapchain::init(GPUContext & context, const RenderingConfig & config){
	_context = context;

	// Query swapchain properties and pick settings.
	// Basic capabilities.
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_context.physicalDevice, _context.surface, &capabilities);
	// Number of items in the swapchain.
	_count = capabilities.minImageCount + 1;
	// maxImageCount = 0 if there is no upper constraint.
	if(capabilities.maxImageCount > 0) {
		_count = std::min(_count, capabilities.maxImageCount);
	}

	// Compute size.
	VkExtent2D extent;
	extent.width = glm::clamp((uint32_t)config.screenResolution.x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	extent.height = glm::clamp((uint32_t)config.screenResolution.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	// Look for a viable format
	// Supported formats.
	std::vector<VkSurfaceFormatKHR> formats;
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(_context.physicalDevice, _context.surface, &formatCount, nullptr);
	formats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(_context.physicalDevice, _context.surface, &formatCount, formats.data());

	// Ideally RGBA8 with a sRGB display.
	VkSurfaceFormatKHR surfaceParams = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	// If only Undefined is present, we can use whatever we want.
	bool useDefaultFormat = (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED);
	// Otherwise, check if our default format is in the list.
	for(const auto& availableFormat : formats) {
		if(availableFormat.format == surfaceParams.format && availableFormat.colorSpace == surfaceParams.colorSpace) {
			useDefaultFormat = true;
			break;
		}
	}
	// Else just take what is given.
	if(!useDefaultFormat){
		surfaceParams = formats[0];
	}

	// Look for a presentation mode.
	// Supported modes.
	std::vector<VkPresentModeKHR> presentModes;
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(_context.physicalDevice, _context.surface, &presentModeCount, nullptr);
	presentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(_context.physicalDevice, _context.surface, &presentModeCount, presentModes.data());

	// By default only FIFO (~V-sync mode) is always available.
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for(const auto& availableMode : presentModes) {
		// If available, we directly pick triple buffering.
		if(availableMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = availableMode;
		}
		if((availableMode == VK_PRESENT_MODE_IMMEDIATE_KHR) && !config.vsync) {
			// Uncapped framerate.
			presentMode = availableMode;
		}
	}

	// Swap chain setup.
	VkSwapchainCreateInfoKHR swapInfo = {};
	swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapInfo.surface = _context.surface;
	swapInfo.minImageCount = _count;
	swapInfo.imageFormat = surfaceParams.format;
	swapInfo.imageColorSpace = surfaceParams.colorSpace;
	swapInfo.imageExtent = extent;
	swapInfo.imageArrayLayers = 1;
	swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// Establish a link with both queues, handling the case where they are the same.
	uint32_t queueFamilyIndices[] = { _context.graphicsId, _context.presentId };
	if(queueFamilyIndices[0] != queueFamilyIndices[1]){
		swapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapInfo.queueFamilyIndexCount = 2;
		swapInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapInfo.queueFamilyIndexCount = 0;
		swapInfo.pQueueFamilyIndices = nullptr;
	}
	swapInfo.preTransform = capabilities.currentTransform;
	swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapInfo.presentMode = presentMode;
	swapInfo.clipped = VK_TRUE;
	swapInfo.oldSwapchain = _swapchain;

	if(vkCreateSwapchainKHR(_context.device, &swapInfo, nullptr, &_swapchain) != VK_SUCCESS) {
		Log::Error() << Log::GPU << "Unable to create swap chain." << std::endl;
		return;
	}

	// Find a proper depth format.
	const VkFormat depthFormat = VkUtils::findSupportedFormat(_context.physicalDevice,  {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	_pass = createMainRenderpass(depthFormat, surfaceParams.format);

	// Create depth buffer.
//	const glm::uvec2 tgtSize(parameters.extent.width, parameters.extent.height;
//							 )
//	VkUtils::createImage(_context, size, 1, depthFormat , VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false, _depthImage, _depthImageMemory);
//	_depthImageView = VkUtils::createImageView(_context, _depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, false, 1);
//	VkUtils::transitionImageLayout(_context, _depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, false, 1);
//
//	// Retrieve images in the swap chain.
//	vkGetSwapchainImagesKHR(_context.device, _swapchain, &count, nullptr);
//	_colors.resize(count);
//	Log::Info() << Log::GPU << "Swapchain using " << count << " images."<< std::endl;
//	vkGetSwapchainImagesKHR(_context.device, _swapchain, &count, _colors.data());
//	// Create views for each image.
//	_colorViews.resize(count);
//	for(size_t i = 0; i < count; i++) {
//		_colorViews[i] = VkUtils::createImageView(device, _colors[i], surfaceInfos.format, VK_IMAGE_ASPECT_COLOR_BIT, false, 1);
//	}
//	// Framebuffers.
//	_colorBuffers.resize(count);
//	for(size_t i = 0; i < count; ++i){
//		std::array<VkImageView, 2> attachments = { _colorViews[i], _depthView };
//
//		VkFramebufferCreateInfo framebufferInfo = {};
//		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
//		framebufferInfo.renderPass = finalRenderPass;
//		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
//		framebufferInfo.pAttachments = attachments.data();
//		framebufferInfo.width = parameters.extent.width;
//		framebufferInfo.height = parameters.extent.height;
//		framebufferInfo.layers = 1;
//		if(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &_colorBuffers[i]) != VK_SUCCESS) {
//			Log::Error() << Log::GPU << "Unable to create swap framebuffers." << std::endl;
//		}
//	}
//
//


	// Only once: Semaphores and fences.
	_imagesAvailable.resize(_count);
	_framesFinished.resize(_count);
	_framesInFlight.resize(_count);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for(size_t i = 0; i < _count; i++) {
		VkResult availRes = vkCreateSemaphore(_context.device, &semaphoreInfo, nullptr, &_imagesAvailable[i]);
		VkResult finishRes = vkCreateSemaphore(_context.device, &semaphoreInfo, nullptr, &_framesFinished[i]);
		VkResult inflightRes = vkCreateFence(_context.device, &fenceInfo, nullptr, &_framesInFlight[i]);

		if(availRes != VK_SUCCESS || finishRes != VK_SUCCESS || inflightRes != VK_SUCCESS){
			Log::Error() << Log::GPU << "Unable to create semaphores and fences." << std::endl;
		}
	}
}

VkRenderPass Swapchain::createMainRenderpass(const VkFormat & depth, const VkFormat & color){
	// We might want to abstract this for all renderpasses based on framebuffer info.
	// Depth attachment.
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = depth;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Color attachment.
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = color;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Subpass.
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	// Dependencies.
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// Render pass.
	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkRenderPass pass = VK_NULL_HANDLE;
	if(vkCreateRenderPass(_context.device, &renderPassInfo, nullptr, &pass) != VK_SUCCESS) {
		Log::Error() << Log::GPU << "Unable to create main render pass." << std::endl;
	}
	return pass;
}
