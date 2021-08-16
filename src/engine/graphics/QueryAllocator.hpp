#pragma once

#include "Common.hpp"
#include "graphics/GPUObjects.hpp"

/** \brief Manages GPU queries allocation in a set of pools. Pools need to be buffered per frame so that we can retrieve 
 * the previous frame queries while the current queries are running.
 * \ingroup Graphics
 */
class QueryAllocator {
public:

	/** Setup the allocator for a given query type.
	 * \param type the type of queries to store
	 * \param count the total number of queries to allocate
	 */
	void init(GPUQuery::Type type, uint count);

	/** Allocate a query and returns its offset in the pool.
	 * \return offset in the pool.
	 *  */
	uint allocate();

	/** 
	 * Reset the pool that will be used at the current frame for new queries.
	 */
	void resetWritePool();

	/** \return the current frame pool (for starting/ending queries) */
	VkQueryPool& getWritePool();

	/** \return the previous frame pool (for retrieving the values) */
	VkQueryPool& getReadPool();

	/** Clean the query pools. */
	void clean();
	
private:

	std::vector<VkQueryPool> _pools; ///< Per-frame native query pools.
	uint _totalCount = 0u; ///< Total size of each pool, in queries.
	uint _currentCount = 0u; ///< Current number of allocated queries.
	uint _itemSize = 1u; ///< Number of hardware queries used for the given query type (two for duration queries, fi).
};
