#pragma once

#include "Common.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GPUObjects.hpp"
#include "system/Config.hpp"

#include "resources/Texture.hpp"

// Forward declarations.
struct GPUContext;

/**
 \brief A swapchain handles the creation and presentation of the backbuffer, along with GPU work submission
 and synchronization. 
 \ingroup Graphics
 */
class Swapchain {

public:

	/** Create a swapchain.
	 * \param context the GPU internal context
	 * \param config the renderer configuration
	 */
	Swapchain(GPUContext & context, const RenderingConfig & config);

	/** Resize an existing swapchain, recreating backbuffers.
	 * \param w the new width
	 * \param h the new height
	 */
	void resize(uint w, uint h);

	/** Submit the current frame's work and start the next one.
	 * \return true if a new frame has been successfully started
	 */
	bool nextFrame();

	/** \return the number of backbuffers handled by the swapchain. */
	uint count(){ return _imageCount; }
	
	/** \return the minimum number of backbuffers required by the swapchain. */
	uint minCount(){ return _minImageCount; }

	/** Destructor. */
	~Swapchain();

	/** \return a renderpass compatible with the swapchain backbuffer. */
	VkRenderPass getRenderPass();

private:

	/** Setup the swapchain for a given size
	 * \param width the new width
	 * \param height the new height
	 */
	void setup(uint32_t width, uint32_t height);

	/** Create the render pass for the swapchain backbuffer
	 * \param depth the depth attachment format
	 * \param color the color attachment format
	 * \return the newly created render pass
	 * \todo Cleanup
	 */
	VkRenderPass createMainRenderpass(const VkFormat & depth, const VkFormat & color);

	/** Submit the current frame's work.
	 * \return true if the submission was successful
	 */
	bool finishFrame();
	
	/** Destroy internal structures. */
	void clean();

	GPUContext* _context = nullptr; ///< The GPU internal context.
	VkSwapchainKHR _swapchain = VK_NULL_HANDLE; ///< Native handle.

	std::vector<std::shared_ptr<Framebuffer>> _framebuffers; ///< Backbuffers.
	Texture _depth; ///< The shared depth texture.

	std::vector<VkSemaphore> _imagesAvailable; ///< Semaphores signaling when swapchain images are available for a new frame.
	std::vector<VkSemaphore> _framesFinished; ///< Semaphores signaling when a frame has been completed.
	std::vector<VkFence> _framesInFlight; ///< Fences ensuring that frames are properly ordered.

	uint _imageCount = 0; ///< Number of images in the swapchain.
	uint _minImageCount = 0; ///< Minimum number of images required by the swapchain.
	bool _vsync = false; ///< Is V-sync enabled.
	uint32_t _imageIndex = 0; ///< Current image index.
	bool _frameStarted = false; ///< Has a frame been previously submitted.
};
