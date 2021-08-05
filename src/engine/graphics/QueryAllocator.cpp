#include "graphics/QueryAllocator.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"

void QueryAllocator::init(GPUQuery::Type type, uint count){
	GPUContext* context = GPU::getInternal();
	_pools.resize(context->frameCount);

	_itemSize = type == GPUQuery::Type::TIME_ELAPSED ? 2 : 1;
	_totalCount = _itemSize * count;

	static const std::map<GPUQuery::Type, VkQueryType> types = {
			{ GPUQuery::Type::TIME_ELAPSED, VK_QUERY_TYPE_TIMESTAMP},
			{ GPUQuery::Type::SAMPLES_DRAWN, VK_QUERY_TYPE_OCCLUSION},
			{ GPUQuery::Type::ANY_DRAWN, VK_QUERY_TYPE_OCCLUSION},
	};
	if(types.count(type) == 0){
		Log::Error() << Log::GPU << "Unsupported query type on this device." << std::endl;
	}
	const VkQueryType rawType = types.at(type);

	for(uint fid = 0; fid < context->frameCount; ++fid){
		VkQueryPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		poolInfo.queryType = rawType;
		poolInfo.queryCount = _totalCount;
		poolInfo.pipelineStatistics = 0;

		if(vkCreateQueryPool(context->device, &poolInfo, nullptr, &_pools[fid]) != VK_SUCCESS){
			Log::Error() << Log::GPU << "Unable to create query pool." << std::endl;
		}
	}
}

uint QueryAllocator::allocate(){
	if(_currentCount >= _totalCount){
		Log::Error() << Log::GPU << "Not enough space left in the query pool." << std::endl;
		return 0;
	}
	const uint start = _currentCount;
	_currentCount += _itemSize;
	return start;
}

void QueryAllocator::clean(){
	GPUContext* context = GPU::getInternal();
	for(VkQueryPool& pool : _pools){
		vkDestroyQueryPool(context->device, pool, nullptr);
	}
	_pools.clear();
}

void QueryAllocator::resetPool(){
	GPUContext* context = GPU::getInternal();
	vkCmdResetQueryPool(context->getCurrentCommandBuffer(), _pools[context->swapIndex], 0, _totalCount);
}

VkQueryPool& QueryAllocator::getCurrentPool(){
	return _pools[GPU::getInternal()->swapIndex];
}

VkQueryPool& QueryAllocator::getPreviousPool(){
	return _pools[1 - GPU::getInternal()->swapIndex];
}
