#include "graphics/Swapchain.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"

#include <map>

Texture* Swapchain::_backbufferStatic = nullptr;

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

	// Create command buffers.
	VkUtils::createCommandBuffers(*_context, _context->frameCount);
	// Immediatly open the first set of command buffers, as it will also
	// be used for swapchain images transitions and data uploads.
	GPU::beginFrameCommandBuffers();

	// Find a proper depth format for the swapchain.
	const Layout depthLayout = VkUtils::findSupportedFormat(_context->physicalDevice,  {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	// Create shared depth buffer.
	_depth.width  = extent.width;
	_depth.height = extent.height;
	_depth.depth  = 1;
	_depth.levels = 1;
	_depth.shape  = TextureShape::D2;

	GPU::setupTexture(_depth, depthLayout, true);
	_depth.gpu->defaultLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkUtils::imageLayoutBarrier(_context->getUploadCommandBuffer(), *_depth.gpu, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0, _depth.levels, 0, _depth.depth);

	// Retrieve image count in the swap chain.
	std::vector<VkImage> colorImages;
	VK_RET(vkGetSwapchainImagesKHR(_context->device, _swapchain, &_imageCount, nullptr));
	colorImages.resize(_imageCount);
	Log::Info() << Log::GPU << "Swapchain using " << _imageCount << " images, requested " << _minImageCount << "."<< std::endl;
	// Retrieve the images themselves.
	VK_RET(vkGetSwapchainImagesKHR(_context->device, _swapchain, &_imageCount, colorImages.data()));

	Layout colorFormat = VkUtils::convertFormat(surfaceParams.format);

	_colors.reserve(_imageCount);
	for(uint i = 0; i < _imageCount; ++i){
		_colors.emplace_back("Color");
		Texture& color = _colors.back();
		color.width = extent.width;
		color.height = extent.height;
		color.depth = 1;
		color.levels = 1;
		color.shape = TextureShape::D2;
		color.gpu.reset(new GPUTexture(colorFormat));
		color.gpu->name = color.name();
		color.gpu->owned = false;
		color.gpu->image = colorImages[i];
		color.gpu->layouts.resize(1, std::vector<VkImageLayout>(1, VK_IMAGE_LAYOUT_UNDEFINED));
		color.gpu->defaultLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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
		VK_RET(vkCreateImageView(_context->device, &viewInfo, nullptr, &(color.gpu->view)));

		color.gpu->views.resize(1);
		color.gpu->views[0].views.resize(1);
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
		VK_RET(vkCreateImageView(_context->device, &viewInfoMip, nullptr, &(color.gpu->views[0].mipView)));
		VK_RET(vkCreateImageView(_context->device, &viewInfoMip, nullptr, &(color.gpu->views[0].views[0])));

	}

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

void Swapchain::getFormats(VkFormat& color, VkFormat& depth, VkFormat& stencil){
	color = _colors[0].gpu->format;
	depth = _depth.gpu->format;
	if(_depth.gpu->typedFormat == Layout::DEPTH24_STENCIL8 || _depth.gpu->typedFormat == Layout::DEPTH32F_STENCIL8){
		stencil = depth;
	} else {
		stencil = VK_FORMAT_UNDEFINED;
	}
}

bool Swapchain::finishFrame(){

	GPU::unbindFramebufferIfNeeded();

	// If we have upload operations to perform, ensure they are all complete before
	// we start executing the render command buffer.
	vkCmdPipelineBarrier(_context->getUploadCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

	// Make sure that the backbuffer is presentable.
	VkUtils::imageLayoutBarrier(_context->getRenderCommandBuffer(), *(_backbuffer->gpu), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, 1, 0, 1);

	// Finish the command buffers for this frame.
	VK_RET(vkEndCommandBuffer(_context->getRenderCommandBuffer()));
	VK_RET(vkEndCommandBuffer(_context->getUploadCommandBuffer()));

	const VkCommandBuffer commandBuffers[] = {_context->getUploadCommandBuffer(), _context->getRenderCommandBuffer()};

	// Submit both command buffers.
	const uint swapIndex = _context->swapIndex;
	// Maybe use VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT instead ?
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &_imagesAvailable[swapIndex];
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 2;
	submitInfo.pCommandBuffers = &commandBuffers[0];
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
	}  else {
		// Before the first frame, we might still have performed upload operations (loading debug data for instance).
		// End the command buffers, submit and wait on queue.
		GPU::submitFrameCommandBuffers();
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

	// Prepare command buffers for this frame.
	GPU::beginFrameCommandBuffers();
	
	_frameStarted = true;
	_backbuffer = &_colors[_imageIndex];
	_backbufferStatic = _backbuffer;

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
		vkDestroyImageView(_context->device, _colors[i].gpu->view, nullptr);
		vkDestroyImageView(_context->device, _colors[i].gpu->views[0].mipView, nullptr);
		vkDestroyImageView(_context->device, _colors[i].gpu->views[0].views[0], nullptr);
	}
	_colors.clear();

	// We own the shared depth, clean it.
	_depth.clean();

	vkFreeCommandBuffers(_context->device, _context->commandPool, uint32_t(_context->renderCommandBuffers.size()), _context->renderCommandBuffers.data());
	vkFreeCommandBuffers(_context->device, _context->commandPool, uint32_t(_context->uploadCommandBuffers.size()), _context->uploadCommandBuffers.data());
	vkDestroySwapchainKHR(_context->device, _swapchain, nullptr);

	for(size_t i = 0; i < _framesFinished.size(); ++i) {
		vkDestroySemaphore(_context->device, _framesFinished[i], nullptr);
		vkDestroySemaphore(_context->device, _imagesAvailable[i], nullptr);
		vkDestroyFence(_context->device, _framesInFlight[i], nullptr);
	}

}
