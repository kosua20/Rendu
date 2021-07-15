#pragma once

#include "Common.hpp"
#include <unordered_map>

#include <volk/volk.h>


class PipelineCache {
public:

	void init();

	VkPipeline getPipeline(const GPUState & state);

	void clean();
	
private:

	struct Entry {
		VkPipeline pipeline;
		const GPUMesh* mesh;
		const Program* program;
		// Framebuffer* framebuffer;
	};

	VkPipeline createNewPipeline(const GPUState& state, const uint64_t hash);

	VkPipeline buildPipeline(const GPUState& state);

	using ProgramPipelines = std::unordered_multimap<uint64_t, Entry>;
	using Cache = std::unordered_map<const Program*, ProgramPipelines>;
	Cache _pipelines;
	VkPipelineCache _vulkanCache = VK_NULL_HANDLE;
};
