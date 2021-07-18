#include "graphics/Swapchain.hpp"
#include "graphics/GPU.hpp"


Swapchain::Swapchain() : _depthTexture("Backbuffer depth"){
	_imageIndex = 0;
}


void Swapchain::init(GPUContext & context, const RenderingConfig & config){
	_context = &context;
	_vsync = config.vsync;

	setup(config.screenResolution.x, config.screenResolution.y);
}

void Swapchain::setup(uint32_t width, uint32_t height){
	_frameStarted = false;
	// Query swapchain properties and pick settings.
	// Basic capabilities.
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_context->physicalDevice, _context->surface, &capabilities);
	// We want three images in our swapchain.
	_minImageCount = _imageCount = 3;
	// Number of items in the swapchain.
	if(_imageCount < capabilities.minImageCount){
		Log::Error() << Log::GPU << "Swapchain doesn't allow for " << _imageCount << " images." << std::endl;
		return;
	}
	// maxImageCount = 0 if there is no upper constraint.
	if((capabilities.maxImageCount != 0) && (capabilities.maxImageCount < _imageCount) ){
		Log::Error() << Log::GPU << "Swapchain doesn't allow for " << _imageCount << " images." << std::endl;
		return;
	}

	// Compute size.
	VkExtent2D extent;
	extent.width = glm::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	extent.height = glm::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	// Look for a viable format
	// Supported formats.
	std::vector<VkSurfaceFormatKHR> formats;
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(_context->physicalDevice, _context->surface, &formatCount, nullptr);
	formats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(_context->physicalDevice, _context->surface, &formatCount, formats.data());

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
	vkGetPhysicalDeviceSurfacePresentModesKHR(_context->physicalDevice, _context->surface, &presentModeCount, nullptr);
	presentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(_context->physicalDevice, _context->surface, &presentModeCount, presentModes.data());

	// By default only FIFO (~V-sync mode) is always available.
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for(const auto& availableMode : presentModes) {
		// If available, we directly pick triple buffering.
		if(availableMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = availableMode;
		}
		if((availableMode == VK_PRESENT_MODE_IMMEDIATE_KHR) && !_vsync) {
			// Uncapped framerate.
			presentMode = availableMode;
		}
	}

	// Swap chain setup.
	VkSwapchainCreateInfoKHR swapInfo = {};
	swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapInfo.surface = _context->surface;
	swapInfo.minImageCount = _imageCount;
	swapInfo.imageFormat = surfaceParams.format;
	swapInfo.imageColorSpace = surfaceParams.colorSpace;
	swapInfo.imageExtent = extent;
	swapInfo.imageArrayLayers = 1;
	swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// Establish a link with both queues, handling the case where they are the same.
	uint32_t queueFamilyIndices[] = { _context->graphicsId, _context->presentId };
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
	swapInfo.oldSwapchain = VK_NULL_HANDLE;

	if(vkCreateSwapchainKHR(_context->device, &swapInfo, nullptr, &_swapchain) != VK_SUCCESS) {
		Log::Error() << Log::GPU << "Unable to create swap chain." << std::endl;
		return;
	}

	// Find a proper depth format.
	const VkFormat depthFormat = VkUtils::findSupportedFormat(_context->physicalDevice,  {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	_pass = createMainRenderpass(depthFormat, surfaceParams.format);
	_context->mainRenderPass = _pass;

	// \todo See how to wrap this in a standard framebuffer.

	// Create depth buffer.
	static const std::map<VkFormat, Layout> formatInfos = {
		{ VK_FORMAT_D16_UNORM, Layout::DEPTH_COMPONENT16 },
		{ VK_FORMAT_D32_SFLOAT, Layout::DEPTH_COMPONENT32F },
		{ VK_FORMAT_D24_UNORM_S8_UINT, Layout::DEPTH24_STENCIL8 },
		{ VK_FORMAT_D32_SFLOAT_S8_UINT, Layout::DEPTH32F_STENCIL8 },
	};

	_depthTexture.width  = extent.width;
	_depthTexture.height = extent.height;
	_depthTexture.depth  = 1;
	_depthTexture.levels = 1;
	_depthTexture.shape  = TextureShape::D2;

	Descriptor desc(formatInfos.at(depthFormat), Filter::LINEAR, Wrap::CLAMP);
	GPU::setupTexture(_depthTexture, desc);

	VkCommandBuffer commandBuffer = VkUtils::startOneTimeCommandBuffer(*_context);
	VkUtils::transitionImageLayout(commandBuffer, _depthTexture.gpu->image, _depthTexture.gpu->format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, _depthTexture.levels, _depthTexture.depth);
	VkUtils::endOneTimeCommandBuffer(commandBuffer, *_context);

	// Retrieve image count in the swap chain.
	vkGetSwapchainImagesKHR(_context->device, _swapchain, &_imageCount, nullptr);
	_colors.resize(_imageCount);
	Log::Info() << Log::GPU << "Swapchain using " << _imageCount << " images, requested " << _minImageCount << "."<< std::endl;
	// Retrieve the images themselves.
	vkGetSwapchainImagesKHR(_context->device, _swapchain, &_imageCount, _colors.data());
	// Create views for each image.
	_colorViews.resize(_imageCount);


	for(size_t i = 0; i < _imageCount; i++) {

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = _colors[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = surfaceParams.format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		vkCreateImageView(_context->device, &viewInfo, nullptr, &(_colorViews[i]));

	}

	// Framebuffers.
	_colorBuffers.resize(_imageCount);
	for(size_t i = 0; i < _imageCount; ++i){
		std::array<VkImageView, 2> attachments = { _colorViews[i], _depthTexture.gpu->view };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = _pass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;
		if(vkCreateFramebuffer(_context->device, &framebufferInfo, nullptr, &_colorBuffers[i]) != VK_SUCCESS) {
			Log::Error() << Log::GPU << "Unable to create swap framebuffers." << std::endl;
		}
	}

	// Only once: Semaphores and fences.
	_imagesAvailable.resize(_context->frameCount);
	_framesFinished.resize(_context->frameCount);
	_framesInFlight.resize(_context->frameCount);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for(size_t i = 0; i < _framesFinished.size(); i++) {
		VkResult availRes = vkCreateSemaphore(_context->device, &semaphoreInfo, nullptr, &_imagesAvailable[i]);
		VkResult finishRes = vkCreateSemaphore(_context->device, &semaphoreInfo, nullptr, &_framesFinished[i]);
		VkResult inflightRes = vkCreateFence(_context->device, &fenceInfo, nullptr, &_framesInFlight[i]);

		if(availRes != VK_SUCCESS || finishRes != VK_SUCCESS || inflightRes != VK_SUCCESS){
			Log::Error() << Log::GPU << "Unable to create semaphores and fences." << std::endl;
		}
	}

	VkUtils::createCommandBuffers(*_context, _context->frameCount);
	
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
	if(vkCreateRenderPass(_context->device, &renderPassInfo, nullptr, &pass) != VK_SUCCESS) {
		Log::Error() << Log::GPU << "Unable to create main render pass." << std::endl;
	}
	return pass;
}


void Swapchain::resize(uint width, uint height){
	if(width == _depthTexture.width && height == _depthTexture.height){
		return;
	}
	destroy();
	// Recreate swapchain.
	setup((uint32_t)width, (uint32_t)height);
}

bool Swapchain::finishFrame(){
	// \todo Do outside
	{
		// Finish final pass and command buffer.
		vkCmdEndRenderPass(_context->getCurrentCommandBuffer());
	}

	// Finish the frame command buffer.
	vkEndCommandBuffer(_context->getCurrentCommandBuffer());

	// Submit the last command buffer.
	const uint swapIndex = _context->swapIndex;
	// Maybe use VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT instead ?
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &_imagesAvailable[swapIndex];
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_context->getCurrentCommandBuffer();
	// Semaphore for when the command buffer is done, so that we can present the image.
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &_framesFinished[swapIndex];
	// Add the fence so that we don't reuse the command buffer while it's in use.
	vkResetFences(_context->device, 1, &_framesInFlight[swapIndex]);
	vkQueueSubmit(_context->graphicsQueue, 1, &submitInfo, _framesInFlight[swapIndex]);

	// Present swap chain.
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	// Check for the command buffer to be done.
	presentInfo.pWaitSemaphores = &_framesFinished[swapIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &_swapchain;
	presentInfo.pImageIndices = &_imageIndex;
	VkResult status = vkQueuePresentKHR(_context->presentQueue, &presentInfo);

	// Here we could also be notified of a resize or invalidation.
	if(status == VK_ERROR_OUT_OF_DATE_KHR || status == VK_SUBOPTIMAL_KHR){
		return false;
	}
	return true;
}

bool Swapchain::nextFrame(){

	// Present on swap chain.
	const bool hadPreviousFrame = _frameStarted;

	if(_frameStarted){

		bool valid = finishFrame();
		// Move to next frame in all cases.
		_context->nextFrame();
		_frameStarted = false;
		if(!valid){
			return false;
		}
	}

	// Wait for the current commands buffer to be done.
	vkWaitForFences(_context->device, 1, &_framesInFlight[_context->swapIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());

	// Acquire image from next frame.
	// Use a semaphore to know when the image is available.
	VkResult status = vkAcquireNextImageKHR(_context->device, _swapchain, std::numeric_limits<uint64_t>::max(), _imagesAvailable[_context->swapIndex], VK_NULL_HANDLE, &_imageIndex);

	// Populate infos.
	VkRenderPassBeginInfo infos = {};
	infos.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	infos.renderPass = _pass;
	infos.framebuffer = _colorBuffers[_imageIndex];
	infos.renderArea.offset = { 0, 0 };
	infos.renderArea.extent = {(uint32_t)_depthTexture.width, (uint32_t)_depthTexture.height};

	// We should resize the swapachain.
	if(status != VK_SUCCESS && status != VK_SUBOPTIMAL_KHR) {
		// If VK_SUBOPTIMAL_KHR, we can still render a frame.
		// Immediatly step to the next frame after resize.
		if(!hadPreviousFrame){
			_context->nextFrame();
		}
		return false;
	}

	// prepare command buffer.

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(_context->getCurrentCommandBuffer(), &beginInfo);
	_frameStarted = true;
	
	// Record actions (to be replaced)
	{
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { {0.0f, 1.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };
		infos.clearValueCount = static_cast<uint32_t>(clearValues.size());
		infos.pClearValues = clearValues.data();
		// Submit final pass.
		vkCmdBeginRenderPass(_context->getCurrentCommandBuffer(), &infos, VK_SUBPASS_CONTENTS_INLINE);
		//...
		_context->newRenderPass = true;

	}

	return true;
}

void Swapchain::clean(){
	// Make sure all commands are finished before deleting anything.
	if(_frameStarted){
		_frameStarted = false;
		finishFrame();
	}

	destroy();

	for(size_t i = 0; i < _framesFinished.size(); i++) {
		vkDestroySemaphore(_context->device, _framesFinished[i], nullptr);
		vkDestroySemaphore(_context->device, _imagesAvailable[i], nullptr);
		vkDestroyFence(_context->device, _framesInFlight[i], nullptr);
	}
	
}

void Swapchain::destroy() {
	vkDeviceWaitIdle(_context->device);
	
	for(size_t i = 0; i < _imageCount; i++) {
		vkDestroyFramebuffer(_context->device, _colorBuffers[i], nullptr);
	}
	vkFreeCommandBuffers(_context->device, _context->commandPool, _context->commandBuffers.size(), _context->commandBuffers.data());
	vkDestroyRenderPass(_context->device, _pass, nullptr);
	for(size_t i = 0; i < _imageCount; i++) {
		vkDestroyImageView(_context->device, _colorViews[i], nullptr);
	}
	_depthTexture.clean();
	vkDestroySwapchainKHR(_context->device, _swapchain, nullptr);
}
