#include "graphics/Swapchain.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"

#include <map>

Swapchain::Swapchain(GPUContext & context, const RenderingConfig & config) : _depth("Shared depth") {
	_imageIndex = 0;
	_context = &context;
	_vsync = config.vsync;
	setup(uint32_t(config.screenResolution.x), uint32_t(config.screenResolution.y));

}

void Swapchain::setup(uint32_t width, uint32_t height){
	_frameStarted = false;
	// Query swapchain properties and pick settings.
	// Basic capabilities.
	VkSurfaceCapabilitiesKHR capabilities;
	VK_RET(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_context->physicalDevice, _context->surface, &capabilities));
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
	VK_RET(vkGetPhysicalDeviceSurfaceFormatsKHR(_context->physicalDevice, _context->surface, &formatCount, nullptr));
	formats.resize(formatCount);
	VK_RET(vkGetPhysicalDeviceSurfaceFormatsKHR(_context->physicalDevice, _context->surface, &formatCount, formats.data()));

	// Ideally RGBA8 with a sRGB display. //VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_SRGB
	const VkColorSpaceKHR tgtColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	const std::vector<VkFormat> tgtFormats = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8_SRGB, VK_FORMAT_R8G8B8_SRGB};
	VkSurfaceFormatKHR surfaceParams = {tgtFormats[0], tgtColorSpace};

	// If only Undefined is present, we can use whatever we want.
	bool foundTargetSurfaceParams = (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED);
	// Otherwise, check if our default formats are in the list.
	for(const auto& availableFormat : formats) {
		if(availableFormat.colorSpace != tgtColorSpace) {
			continue;
		}
		if(std::find(tgtFormats.begin(), tgtFormats.end(), availableFormat.format) == tgtFormats.end()){
			continue;
		}
		// We found one, stop.
		surfaceParams.format = availableFormat.format;
		foundTargetSurfaceParams = true;
		break;
	}
	// Else just take what is given.
	if(!foundTargetSurfaceParams){
		Log::Warning() << "Could not found a target surface format, using whatever is available. Gamma issues might appear." << std::endl;
		surfaceParams = formats[0];
	}

	// Look for a presentation mode.
	// By default only FIFO (~V-sync mode) is always available.
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	// If we want an uncapped framerate mode, check if available.
	if(!_vsync){
		// Supported modes.
		std::vector<VkPresentModeKHR> presentModes;
		uint32_t presentModeCount;
		VK_RET(vkGetPhysicalDeviceSurfacePresentModesKHR(_context->physicalDevice, _context->surface, &presentModeCount, nullptr));
		presentModes.resize(presentModeCount);
		VK_RET(vkGetPhysicalDeviceSurfacePresentModesKHR(_context->physicalDevice, _context->surface, &presentModeCount, presentModes.data()));

		for(const auto& availableMode : presentModes) {
			if(availableMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				// Uncapped framerate.
				presentMode = availableMode;
				break;
			}
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
	swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

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


	static const std::map<VkFormat, Layout> formatInfos = {
		{ VK_FORMAT_R8_UNORM, Layout::R8 },
		{ VK_FORMAT_R8G8_UNORM, Layout::RG8 },
		{ VK_FORMAT_R8G8B8A8_UNORM, Layout::RGBA8 },
		{ VK_FORMAT_B8G8R8A8_UNORM, Layout::BGRA8 },
		{ VK_FORMAT_R8G8B8A8_SRGB, Layout::SRGB8_ALPHA8 },
		{ VK_FORMAT_B8G8R8A8_SRGB, Layout::SBGR8_ALPHA8 },
		{ VK_FORMAT_R16_UNORM, Layout::R16 },
		{ VK_FORMAT_R16G16_UNORM, Layout::RG16 },
		{ VK_FORMAT_R16G16B16A16_UNORM, Layout::RGBA16 },
		{ VK_FORMAT_R8_SNORM, Layout::R8_SNORM },
		{ VK_FORMAT_R8G8_SNORM, Layout::RG8_SNORM },
		{ VK_FORMAT_R8G8B8A8_SNORM, Layout::RGBA8_SNORM },
		{ VK_FORMAT_R16_SNORM, Layout::R16_SNORM },
		{ VK_FORMAT_R16G16_SNORM, Layout::RG16_SNORM },
		{ VK_FORMAT_R16_SFLOAT, Layout::R16F },
		{ VK_FORMAT_R16G16_SFLOAT, Layout::RG16F },
		{ VK_FORMAT_R16G16B16A16_SFLOAT, Layout::RGBA16F },
		{ VK_FORMAT_R32_SFLOAT, Layout::R32F },
		{ VK_FORMAT_R32G32_SFLOAT, Layout::RG32F },
		{ VK_FORMAT_R32G32B32A32_SFLOAT, Layout::RGBA32F },
		{ VK_FORMAT_R5G5B5A1_UNORM_PACK16, Layout::RGB5_A1 },
		{ VK_FORMAT_A2R10G10B10_UNORM_PACK32, Layout::A2_RGB10 },
		{ VK_FORMAT_A2B10G10R10_UNORM_PACK32, Layout::A2_BGR10 },
		{ VK_FORMAT_D16_UNORM, Layout::DEPTH_COMPONENT16 },
		{ VK_FORMAT_D32_SFLOAT, Layout::DEPTH_COMPONENT32F },
		{ VK_FORMAT_D24_UNORM_S8_UINT, Layout::DEPTH24_STENCIL8 },
		{ VK_FORMAT_D32_SFLOAT_S8_UINT, Layout::DEPTH32F_STENCIL8 },
		{ VK_FORMAT_R8_UINT, Layout::R8UI },
		{ VK_FORMAT_R16_SINT, Layout::R16I },
		{ VK_FORMAT_R16_UINT, Layout::R16UI },
		{ VK_FORMAT_R32_SINT, Layout::R32I },
		{ VK_FORMAT_R32_UINT, Layout::R32UI },
		{ VK_FORMAT_R8G8_SINT, Layout::RG8I },
		{ VK_FORMAT_R8G8_UINT, Layout::RG8UI },
		{ VK_FORMAT_R16G16_SINT, Layout::RG16I },
		{ VK_FORMAT_R16G16_UINT, Layout::RG16UI },
		{ VK_FORMAT_R32G32_SINT, Layout::RG32I },
		{ VK_FORMAT_R32G32_UINT, Layout::RG32UI },
		{ VK_FORMAT_R8G8B8A8_SINT, Layout::RGBA8I },
		{ VK_FORMAT_R8G8B8A8_UINT, Layout::RGBA8UI },
		{ VK_FORMAT_R16G16B16A16_SINT, Layout::RGBA16I },
		{ VK_FORMAT_R16G16B16A16_UINT, Layout::RGBA16UI },
		{ VK_FORMAT_R32G32B32A32_SINT, Layout::RGBA32I },
		{ VK_FORMAT_R32G32B32A32_UINT, Layout::RGBA32UI }
	};

	// Create shared depth buffer.
	_depth.width  = extent.width;
	_depth.height = extent.height;
	_depth.depth  = 1;
	_depth.levels = 1;
	_depth.shape  = TextureShape::D2;


	Descriptor desc(formatInfos.at(depthFormat), Filter::LINEAR, Wrap::CLAMP);
	GPU::setupTexture(_depth, desc, true);
	_depth.gpu->defaultLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkCommandBuffer commandBuffer = VkUtils::startOneTimeCommandBuffer(*_context);
	VkUtils::imageLayoutBarrier(commandBuffer, *_depth.gpu, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0, _depth.levels, 0, _depth.depth);
	VkUtils::endOneTimeCommandBuffer(commandBuffer, *_context);

	// Retrieve image count in the swap chain.
	std::vector<VkImage> colorImages;
	VK_RET(vkGetSwapchainImagesKHR(_context->device, _swapchain, &_imageCount, nullptr));
	colorImages.resize(_imageCount);
	Log::Info() << Log::GPU << "Swapchain using " << _imageCount << " images, requested " << _minImageCount << "."<< std::endl;
	// Retrieve the images themselves.
	VK_RET(vkGetSwapchainImagesKHR(_context->device, _swapchain, &_imageCount, colorImages.data()));

	Descriptor colorDesc(formatInfos.at(surfaceParams.format), Filter::LINEAR, Wrap::CLAMP);

	_framebuffers.resize(_imageCount);

	for(uint i = 0; i < _imageCount; ++i){
		_framebuffers[i].reset(new Framebuffer());
		
		Framebuffer& fb = *_framebuffers[i];

		fb._width = extent.width;
		fb._height = extent.height;
		fb._mips = 1;
		fb._layers = 1;
		fb._shape = TextureShape::D2;
		fb._name = "Backbuffer " + std::to_string(i);
		fb._hasDepth = true;
		fb._isBackbuffer = true;
		
		fb._depth = Texture("Depth");
		fb._depth.width = extent.width;
		fb._depth.height = extent.height;
		fb._depth.depth = 1;
		fb._depth.levels = 1;
		fb._depth.shape = TextureShape::D2;
		fb._depth.gpu.reset(new GPUTexture(_depth.gpu->descriptor()));
		fb._depth.gpu->name = fb._depth.name();
		fb._depth.gpu->owned = false;
		fb._depth.gpu->image = _depth.gpu->image;
		fb._depth.gpu->data = _depth.gpu->data;
		fb._depth.gpu->view = _depth.gpu->view;
		fb._depth.gpu->levelViews = _depth.gpu->levelViews;
		fb._depth.gpu->layouts = _depth.gpu->layouts;
		fb._depth.gpu->sampler = _depth.gpu->sampler;
		fb._depth.gpu->defaultLayout = _depth.gpu->defaultLayout;

		fb._colors.clear();
		fb._colors.emplace_back("Color");
		fb._colors[0].width = extent.width;
		fb._colors[0].height = extent.height;
		fb._colors[0].depth = 1;
		fb._colors[0].levels = 1;
		fb._colors[0].shape = TextureShape::D2;
		fb._colors[0].gpu.reset(new GPUTexture(colorDesc));
		fb._colors[0].gpu->name = fb._colors[0].name();
		fb._colors[0].gpu->owned = false;
		fb._colors[0].gpu->image = colorImages[i];
		fb._colors[0].gpu->layouts.resize(1, std::vector<VkImageLayout>(1, VK_IMAGE_LAYOUT_UNDEFINED));
		fb._colors[0].gpu->sampler = VK_NULL_HANDLE;
		fb._colors[0].gpu->defaultLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = colorImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = surfaceParams.format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		VK_RET(vkCreateImageView(_context->device, &viewInfo, nullptr, &(fb._colors[0].gpu->view)));

		fb._colors[0].gpu->levelViews.resize(1);
		VkImageViewCreateInfo viewInfoMip = {};
		viewInfoMip.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfoMip.image = colorImages[i];
		viewInfoMip.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfoMip.format = surfaceParams.format;
		viewInfoMip.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfoMip.subresourceRange.baseMipLevel = 0;
		viewInfoMip.subresourceRange.levelCount = 1;
		viewInfoMip.subresourceRange.baseArrayLayer = 0;
		viewInfoMip.subresourceRange.layerCount = 1;
		VK_RET(vkCreateImageView(_context->device, &viewInfoMip, nullptr, &(fb._colors[0].gpu->levelViews[0])));

		// Generate the render passes.
		fb.populateRenderPasses(false);
		fb.populateLayoutState();
		
		fb._framebuffers.resize(1);
		fb._framebuffers[0].resize(1);
		fb._framebuffers[0][0].attachments = { fb._colors[0].gpu->view, fb._depth.gpu->view };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = fb._renderPasses[0][0][0];
		framebufferInfo.attachmentCount = static_cast<uint32_t>(fb._framebuffers[0][0].attachments.size());
		framebufferInfo.pAttachments = fb._framebuffers[0][0].attachments.data();
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;
		if(vkCreateFramebuffer(_context->device, &framebufferInfo, nullptr, &fb._framebuffers[0][0].framebuffer) != VK_SUCCESS) {
			Log::Error() << Log::GPU << "Unable to create swap framebuffers." << std::endl;
		}
	}

	VkUtils::createCommandBuffers(*_context, _context->frameCount);

	// Semaphores and fences.
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
	
}

void Swapchain::resize(uint width, uint height){
	if(width == _depth.width && height == _depth.height){
		return;
	}
	clean();
	// Recreate swapchain.
	setup((uint32_t)width, (uint32_t)height);
}

VkRenderPass Swapchain::getRenderPass(){
	return _framebuffers[0]->getRenderPass();
}

bool Swapchain::finishFrame(){

	GPU::unbindFramebufferIfNeeded();

	// Make sure that the backbuffer is presentable.
	VkUtils::imageLayoutBarrier(_context->getCurrentCommandBuffer(), *(Framebuffer::backbuffer()->texture(0)->gpu), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, 1, 0, 1);

	// Finish the frame command buffer.
	VK_RET(vkEndCommandBuffer(_context->getCurrentCommandBuffer()));

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
	VK_RET(vkResetFences(_context->device, 1, &_framesInFlight[swapIndex]));
	VK_RET(vkQueueSubmit(_context->graphicsQueue, 1, &submitInfo, _framesInFlight[swapIndex]));

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
		GPU::nextFrame();
		_frameStarted = false;
		if(!valid){
			return false;
		}
	}

	// Wait for the current commands buffer to be done.
	VK_RET(vkWaitForFences(_context->device, 1, &_framesInFlight[_context->swapIndex], VK_TRUE, std::numeric_limits<uint64_t>::max()));

	// Acquire image from next frame.
	// Use a semaphore to know when the image is available.
	VkResult status = vkAcquireNextImageKHR(_context->device, _swapchain, std::numeric_limits<uint64_t>::max(), _imagesAvailable[_context->swapIndex], VK_NULL_HANDLE, &_imageIndex);


	// We should resize the swapachain.
	if(status != VK_SUCCESS && status != VK_SUBOPTIMAL_KHR) {
		// If VK_SUBOPTIMAL_KHR, we can still render a frame.
		// Immediatly step to the next frame after resize.
		if(!hadPreviousFrame){
			_context->nextFrame();
		}
		return false;
	}

	// Prepare command buffer.
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VK_RET(vkBeginCommandBuffer(_context->getCurrentCommandBuffer(), &beginInfo));
	_frameStarted = true;
	Framebuffer::_backbuffer = _framebuffers[_imageIndex].get();

	// Reset queries for the current frame (we need the command buffer to be active).
	for(auto& alloc : _context->queryAllocators){
		alloc.second.resetWritePool();
	}

	return true;
}

Swapchain::~Swapchain(){
	// Make sure all commands are finished before deleting anything.
	if(_frameStarted){
		_frameStarted = false;
		finishFrame();
	}

	clean();
}

void Swapchain::clean() {
	// Wait for all queues to be idle.
	VK_RET(vkDeviceWaitIdle(_context->device));

	// We have to manually delete the framebuffers, because they don't own their color images (system created) nor their depth texture (shared).
	for(size_t i = 0; i < _imageCount; ++i) {
		// Destroy the view but not the image, as we don't own it (and there is no sampler).
		vkDestroyImageView(_context->device, _framebuffers[i]->_colors[0].gpu->view, nullptr);
		vkDestroyImageView(_context->device, _framebuffers[i]->_colors[0].gpu->levelViews[0], nullptr);
	}
	_framebuffers.clear();

	// We own the shared depth, clean it.
	_depth.clean();

	vkFreeCommandBuffers(_context->device, _context->commandPool, uint32_t(_context->commandBuffers.size()), _context->commandBuffers.data());
	vkDestroySwapchainKHR(_context->device, _swapchain, nullptr);

	for(size_t i = 0; i < _framesFinished.size(); ++i) {
		vkDestroySemaphore(_context->device, _framesFinished[i], nullptr);
		vkDestroySemaphore(_context->device, _imagesAvailable[i], nullptr);
		vkDestroyFence(_context->device, _framesInFlight[i], nullptr);
	}

}
