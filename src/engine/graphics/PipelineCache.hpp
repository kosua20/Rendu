#pragma once

#include "Common.hpp"
#include <unordered_map>

#include <volk/volk.h>


class PipelineCache {
public:

	VkPipeline getPipeline(const GPUState & state);

private:

	struct Entry {
		VkPipeline pipeline;
		const GPUMesh* mesh;
		const Program* program;
		// Framebuffer* framebuffer;
	};

	VkPipeline buildPipeline(const GPUState& state);

	using Cache = std::unordered_multimap<uint64_t, Entry>;
	Cache _pipelines;
};
