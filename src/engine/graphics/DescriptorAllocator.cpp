#include "graphics/DescriptorAllocator.hpp"
#include "graphics/GPUInternal.hpp"

#define DEFAULT_SET_COUNT 1000

void DescriptorAllocator::init(GPUContext* context, uint poolCount){
	_context = context;
	_currentPoolCount = 0;
	_maxPoolCount = poolCount;
	
	_pools.push_back(createPool(DEFAULT_SET_COUNT));
	_imguiPool = createPool(8);

}

VkDescriptorSet DescriptorAllocator::allocateSet(VkDescriptorSetLayout& setLayout){

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &setLayout;
	allocInfo.descriptorPool = _pools.back();

	VkDescriptorSet set;
	if(vkAllocateDescriptorSets(_context->device, &allocInfo, &set) != VK_SUCCESS) {
		VkDescriptorPool newPool = createPool(DEFAULT_SET_COUNT);

		if(newPool == VK_NULL_HANDLE){
			// No space left, try to reuse an old pool.
			Log::Warning() << "Unable to allocate new pool, recycling oldest one." << std::endl;
			VkDescriptorPool oldestPool = _pools.front();
			vkResetDescriptorPool(_context->device, oldestPool, 0);
			_pools.pop_front();
			newPool = oldestPool;
		}

		_pools.push_back(newPool);
		// Try to allocate in newly create pool.
		allocInfo.descriptorPool = _pools.back();
		if(vkAllocateDescriptorSets(_context->device, &allocInfo, &set) != VK_SUCCESS) {
			Log::Error() << "Allocation failed." << std::endl;
			return VK_NULL_HANDLE;
		}
	}

	return set;
}

void DescriptorAllocator::clean(){
	for(auto& pool : _pools){
		vkDestroyDescriptorPool(_context->device, pool, nullptr);
	}
	_pools.clear();
	vkDestroyDescriptorPool(_context->device, _imguiPool, nullptr);
	_imguiPool = VK_NULL_HANDLE;
}


VkDescriptorPool DescriptorAllocator::createPool(uint count){
	if(_currentPoolCount > _maxPoolCount){
		return VK_NULL_HANDLE;
	}
	++_currentPoolCount;

	VkDescriptorPoolSize poolSizes[] =
	{
	//	{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, count },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, count },
	//	{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, count },
	//	{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, count },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count },
	//	{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, count },
	//	{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, count },
	//	{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, count }
	};
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = count * IM_ARRAYSIZE(poolSizes);
	poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
	poolInfo.pPoolSizes = poolSizes;
	VkDescriptorPool pool;
	if(vkCreateDescriptorPool(_context->device, &poolInfo, nullptr, &pool) != VK_SUCCESS){
		return VK_NULL_HANDLE;
	}
	return pool;
}
