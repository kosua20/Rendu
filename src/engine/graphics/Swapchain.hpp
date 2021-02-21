#pragma once

#include "Common.hpp"
#include "graphics/GPUInternal.hpp"
#include "system/Config.hpp"

/**
 \brief
 \ingroup Graphics
 */
class Swapchain {

public:

	Swapchain();

	void init(GPUContext & context, const RenderingConfig & config);

	uint count(){ return _count; }
	
private:

	VkRenderPass createMainRenderpass(const VkFormat & depth, const VkFormat & color);


	GPUContext _context;
	VkSwapchainKHR _swapchain = VK_NULL_HANDLE;

	std::vector<VkSemaphore> _imagesAvailable;
	std::vector<VkSemaphore> _framesFinished;
	std::vector<VkFence> _framesInFlight;

	VkRenderPass _pass;
	uint _count;
};
