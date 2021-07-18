#pragma once

#include "Common.hpp"

#include <volk/volk.h>
#include <deque>

struct GPUContext;

class DescriptorAllocator {
public:

	void init(GPUContext* context, uint poolCount);

	VkDescriptorSet allocateSet(VkDescriptorSetLayout& setLayout);

	void clean();

	VkDescriptorPool getImGuiPool(){ return _imguiPool; }
	
private:

	VkDescriptorPool createPool(uint count);

	GPUContext* _context = nullptr;
	std::deque<VkDescriptorPool> _pools;
	VkDescriptorPool _imguiPool = VK_NULL_HANDLE;
	uint _maxPoolCount = 2;
	uint _currentPoolCount = 0;
};
