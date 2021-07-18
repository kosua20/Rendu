#pragma once

#include "Common.hpp"
#include "graphics/GPUInternal.hpp"
#include "system/Config.hpp"

#include "resources/Texture.hpp"

/**
 \brief
 \ingroup Graphics
 */
class Swapchain {

public:

	Swapchain();

	void init(GPUContext & context, const RenderingConfig & config);

	void resize(uint w, uint h);

	bool nextFrame();

	uint count(){ return _imageCount; }
	// \todo count might be enough if double buffered
	uint minCount(){ return _minImageCount; }

	void clean();

	// \todo leaking Vulkan here.
	const VkRenderPass& getMainPass() const { return _pass; }
	
private:

	void setup(uint32_t width, uint32_t height);

	VkRenderPass createMainRenderpass(const VkFormat & depth, const VkFormat & color);

	bool finishFrame();
	
	void destroy();

	GPUContext* _context;
	VkSwapchainKHR _swapchain = VK_NULL_HANDLE;

	std::vector<VkImage> _colors;
	std::vector<VkImageView> _colorViews;
	std::vector<VkFramebuffer> _colorBuffers;

	Texture _depthTexture;

	std::vector<VkSemaphore> _imagesAvailable;
	std::vector<VkSemaphore> _framesFinished;
	std::vector<VkFence> _framesInFlight;

	VkRenderPass _pass;
	uint _imageCount = 0;
	uint _minImageCount = 0;
	bool _vsync = false;
	uint32_t _imageIndex = 0;
	bool _frameStarted = false;
};
