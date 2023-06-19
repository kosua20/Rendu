#pragma once

#include "Common.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GPUObjects.hpp"

#include <unordered_map>
#include <deque>

/** \brief Create and reuse GPU pipelines based on a given state. This supports both graphics and compute pipelines.
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

	/** Retrieve a pipeline for a given GPU graphics state, or create it if needed.
	 * \param state the state to represent
	 * \return the corresponding native pipeline
	 */
	VkPipeline getGraphicsPipeline(const GPUState & state);

	/** Retrieve a pipeline for a given GPU compute state, or create it if needed.
	 * \param state the state to represent
	 * \return the corresponding native pipeline
	 */
	VkPipeline getComputePipeline(const GPUState & state);

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
		GPUState::FramebufferInfos pass; ///< The framebuffer layout.
	};

	/** Create a new pipeline based on a given state and store it in the cache for future use.
	 * \param state the state to use
	 * \param hash a hash of the complete state (already computed before)
	 * \return the newly created pipeline
	 */
	VkPipeline createNewPipeline(const GPUState& state, const uint64_t hash);

	/** Build a graphics pipeline from a given graphics state.
	 * \param state the state to use
	 * \return the newly created pipeline
	 */
	VkPipeline buildGraphicsPipeline(const GPUState& state);

	/** Build a compute pipeline from a given compute program.
	 * \param program the program to use
	 * \return the newly created pipeline
	 */
	VkPipeline buildComputePipeline(Program& program);

	using ProgramPipelines = std::unordered_multimap<uint64_t, Entry>; ///< Per program pipeline type.
	using GraphicCache = std::unordered_map<const Program*, ProgramPipelines>; ///< Complete cache type.
	using ComputeCache = std::unordered_map<const Program*, VkPipeline>; ///< Complete cache type.

	GraphicCache _graphicPipelines; ///< Graphics pipeline cache.
	ComputeCache _computePipelines; ///< Compute pipeline cache.
	VkPipelineCache _vulkanCache = VK_NULL_HANDLE; ///< Vulkan pipeline cache.

	/** \brief Information for buffered pipeline deletion. */
	struct PipelineToDelete {
		VkPipeline pipeline; ///< The native handle.
		uint64_t frame = 0; ///< The frame at which the deletion request was created.
	};

	std::deque<PipelineToDelete> _pipelinesToDelete; ///< List of pipelines to delete once we are sure they are unused.
};
