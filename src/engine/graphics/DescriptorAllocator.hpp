#pragma once

#include "Common.hpp"
#include "graphics/GPUObjects.hpp"

#include <deque>

struct GPUContext;

/** \brief Manage descriptor set allocations by creating and reusing internal descriptor pools.
 \details By default each pool will contain up to DEFAULT_SET_COUNT of each kind defined in createPool.
 \ingroup Graphics
 */
class DescriptorAllocator {
public:

	/** Setup the allocator.
	 \param context the GPU context
	 \param poolCount the maximum number of pools to allocate
	 */
	void init(GPUContext* context, uint poolCount);

	/** Allocate a descriptor set from an available pool, using the specified layout.
	 \param setLayout the layout to use
	 \return the allocated descriptor set info
	 */
	DescriptorSet allocateSet(VkDescriptorSetLayout& setLayout);

	/** Mark an allocated descriptor set as unused
	 \param set the set to free
	 */
	void freeSet(const DescriptorSet& set);

	/** Reset all descriptor pools */
	void clean();

	/** \return the ImGui dedicated descriptor pool */
	VkDescriptorPool getImGuiPool(){ return _imguiPool.handle; }
	
private:

	/** \brief Descriptor pool management info. */
	struct DescriptorPool {
		VkDescriptorPool handle = VK_NULL_HANDLE; ///< Native handle.
		uint64_t lastFrame = 0; ///< Last frame used.
		uint allocated = 0; ///< Number of currently used descriptors.
		uint id = 0; ///< Pool id.
	};

	/** Create a descriptor pool containing count descriptors.
	 \param count the maximmum number of descriptors of each type to store in the poll
	 \param combined should images and samplers be represented by combined descriptors or separate sampled image/sampler descriptors.
	 \return descriptor pool info
	 */
	DescriptorPool createPool(uint count, bool combined);

	GPUContext* _context = nullptr; ///< The GPU context.
	std::deque<DescriptorPool> _pools; ///< Available pools.
	DescriptorPool _imguiPool; ///< ImGui dedicated pool.

	uint _maxPoolCount = 2; ///< Maximum number of pools to create.
	uint _currentPoolCount = 0; ///< Current number of created pools.
};
