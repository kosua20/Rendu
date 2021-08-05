#pragma once

#include "Common.hpp"
#include "graphics/GPUObjects.hpp"

#include <volk/volk.h>


class QueryAllocator {
public:

	void init(GPUQuery::Type type, uint count);

	uint allocate();

	void resetPool();

	VkQueryPool& getCurrentPool();

	VkQueryPool& getPreviousPool();

	void clean();
	
private:

	std::vector<VkQueryPool> _pools;
	uint _totalCount = 0u;
	uint _currentCount = 0u;
	uint _itemSize = 1u;
};
