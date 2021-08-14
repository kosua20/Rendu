#pragma once

#include "Common.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GPUObjects.hpp"
#include "system/Config.hpp"

#include "resources/Texture.hpp"

struct GPUContext;
/**
 \brief
 \ingroup Graphics
 */
class Swapchain {

public:

	Swapchain(GPUContext & context, const RenderingConfig & config);

	void resize(uint w, uint h);

	bool nextFrame();

	uint count(){ return _imageCount; }
	
	uint minCount(){ return _minImageCount; }

	~Swapchain();

	VkRenderPass getRenderPass();

private:

	void setup(uint32_t width, uint32_t height);

	VkRenderPass createMainRenderpass(const VkFormat & depth, const VkFormat & color);

	bool finishFrame();
	
	void clean();

	GPUContext* _context;
	VkSwapchainKHR _swapchain = VK_NULL_HANDLE;

	std::vector<std::shared_ptr<Framebuffer>> _framebuffers;
	Texture _depth;

	std::vector<VkSemaphore> _imagesAvailable;
	std::vector<VkSemaphore> _framesFinished;
	std::vector<VkFence> _framesInFlight;

	uint _imageCount = 0;
	uint _minImageCount = 0;
	bool _vsync = false;
	uint32_t _imageIndex = 0;
	bool _frameStarted = false;
};
