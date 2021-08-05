#pragma once

#include "Common.hpp"
#include <unordered_map>
#include <deque>

#include <volk/volk.h>


class PipelineCache {
public:

	void init();

	VkPipeline getPipeline(const GPUState & state);

	void freeOutdatedPipelines();

	void clean();
	
private:

	struct Entry {
		VkPipeline pipeline;
		GPUMesh::InputState mesh;
		Framebuffer::LayoutState framebuffer;
		const Program* program;
	};

	VkPipeline createNewPipeline(const GPUState& state, const uint64_t hash);

	VkPipeline buildPipeline(const GPUState& state);

	using ProgramPipelines = std::unordered_multimap<uint64_t, Entry>;
	using Cache = std::unordered_map<const Program*, ProgramPipelines>;
	Cache _pipelines;
	VkPipelineCache _vulkanCache = VK_NULL_HANDLE;

	struct PipelineToDelete {
		VkPipeline pipeline;
		uint64_t frame = 0;
	};

	std::deque<PipelineToDelete> _pipelinesToDelete;
};
