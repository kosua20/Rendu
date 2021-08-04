#pragma once

#include "Common.hpp"
#include "graphics/GPUObjects.hpp"

#include <volk/volk.h>
#include <deque>

struct GPUContext;

class DescriptorAllocator {
public:

	void init(GPUContext* context, uint poolCount);

	DescriptorSet allocateSet(VkDescriptorSetLayout& setLayout);

	void freeSet(const DescriptorSet& set);

	void clean();

	VkDescriptorPool getImGuiPool(){ return _imguiPool.handle; }
	
private:

	struct DescriptorPool {
		VkDescriptorPool handle = VK_NULL_HANDLE;
		uint64_t lastFrame = 0;
		uint allocated = 0;
		uint id = 0;
	};

	DescriptorPool createPool(uint count);

	GPUContext* _context = nullptr;
	std::deque<DescriptorPool> _pools;
	DescriptorPool _imguiPool;

	uint _maxPoolCount = 2;
	uint _currentPoolCount = 0;
};
