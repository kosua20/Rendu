#pragma once

#include "Common.hpp"
#include "graphics/Framebuffer.hpp"

#include <unordered_map>
#include <deque>

#include <volk/volk.h>

/** \brief Create and reuse GPU pipelines based on a given state. 
 * \details We use a two-levels cache, first sorting by Program because each program only has one instance 
 * (and thus one pointer adress) that we can use directly to retrieve pipelines. Then we use a hash of the GPU state 
 * parameters to retrieve compatible pipelines, and compare mesh and framebuffer layouts manually as duplicates will be quite rare.
 * (usually a program is used with a specific set of meshes and a fixed framebuffer). 
 * We could compare program layouts to share pipelines between similar programs, but it would be more complex.
 * A Vulkan cache is also used internally, saved on disk and restored at loading.
 * \ingroup Graphics
 */
class PipelineCache {
public:

	/** Initialize the cache. */
	void init();

	/** Retrieve a pipeline for a given GPU state, or create it if needed.
	 * \param state the state to represent
	 * \return the corresponding native pipeline
	 */
	VkPipeline getPipeline(const GPUState & state);

	/// Destroy pipelines that are referencing outdated state and are not used anymore. 
	void freeOutdatedPipelines();

	/** Clean all existing pipelines */
	void clean();
	
private:

	/** \brief Store a pipeline along with part of the information used to generate it
	 * */
	struct Entry {
		VkPipeline pipeline; ///< The native handle.
		GPUMesh::State mesh; ///< The mesh layout.
		Framebuffer::State framebuffer; ///< The framebuffer layout.
		const Program* program; ///< The program used.
	};

	/** Create a new pipeline based on a given state and store it in the cache for future use.
	 * \param state the state to use
	 * \param hash a hash of the complete state (already computed before)
	 * \return the newly created pipeline
	 */
	VkPipeline createNewPipeline(const GPUState& state, const uint64_t hash);

	/** Build a pipeline from a given state.
	 * \param state the state to use
	 * \return the newly created pipeline
	 */
	VkPipeline buildPipeline(const GPUState& state);

	using ProgramPipelines = std::unordered_multimap<uint64_t, Entry>; ///< Per program pipeline type.
	using Cache = std::unordered_map<const Program*, ProgramPipelines>; ///< Complete cache type.

	Cache _pipelines; ///< Pipeline cache.
	VkPipelineCache _vulkanCache = VK_NULL_HANDLE; ///< Vulkan pipeline cache.

	/** \brief Information for buffered pipeline deletion. */
	struct PipelineToDelete {
		VkPipeline pipeline; ///< The native handle.
		uint64_t frame = 0; ///< The frame at which the deletion request was created.
	};

	std::deque<PipelineToDelete> _pipelinesToDelete; ///< List of pipelines to delete once we are sure they are unused.
};
