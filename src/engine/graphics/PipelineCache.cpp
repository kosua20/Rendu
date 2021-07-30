#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"
#include "graphics/PipelineCache.hpp"

#include "resources/ResourcesManager.hpp"

#define XXH_INLINE_ALL
#include <xxhash/xxhash.h>

#define PIPELINE_CACHE_FILE "pipeline_cache_vulkan.bin"

void PipelineCache::init(){
	GPUContext* context = GPU::getInternal();

	size_t pipelineSize = 0;
	char * pipelineData = Resources::loadRawDataFromExternalFile(PIPELINE_CACHE_FILE, pipelineSize);
	
	VkPipelineCacheCreateInfo cacheInfos{};
	cacheInfos.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	cacheInfos.flags = 0;
	cacheInfos.initialDataSize = pipelineSize;
	cacheInfos.pInitialData = pipelineData;

	vkCreatePipelineCache(context->device, &cacheInfos, nullptr, &_vulkanCache);
	free(pipelineData);
}

VkPipeline PipelineCache::getPipeline(const GPUState & state){
	// Compute the hash (used in all cases).
	const uint64_t hash = XXH3_64bits(&state, offsetof(GPUState, sentinel));

	// First check if we already have pipelines for the current program.
	auto sameProgramPipelinesIt = _pipelines.find(state.program);

	const bool programReloaded = state.program->reloaded(true);
	// We have to invalidate program pipelines after a reload, as the layout might change.
	if(sameProgramPipelinesIt != _pipelines.end() && programReloaded){
		// Delete the pipelines corresponding to this program.
		// If we immediatly destroy a pipeline that was in use earlier in the frame, we might get a crash.
		// So instead schedule the deletion and remove the records.
		for(auto& pipelineInfo : sameProgramPipelinesIt->second){
			_pipelinesToDelete.push_back(pipelineInfo.second.pipeline);
		}
		sameProgramPipelinesIt->second.clear();
		_pipelines.erase(state.program);
		sameProgramPipelinesIt = _pipelines.end();
	}


	// If not found, create new program cache and generate pipeline.
	if(sameProgramPipelinesIt == _pipelines.end()){
		_pipelines[state.program] = ProgramPipelines();
		return createNewPipeline(state, hash);
	}

	const auto& sameProgramPipelines = sameProgramPipelinesIt->second;
	// Else, query all program pipelines with the same state.
	auto sameStatePipelines = sameProgramPipelines.equal_range(hash);
	// If not found, create a new pipeline in the existing program cache.
	if(sameStatePipelines.first == sameProgramPipelines.end()){
		return createNewPipeline(state, hash);
	}
	// Else, find a pipeline with the same mesh layout.
	for(auto pipeline = sameStatePipelines.first; pipeline != sameStatePipelines.second; ++pipeline){
		const Entry& entry = pipeline->second;

		// Test mesh layout compatibility.
		if(!entry.mesh.isEquivalent(state.mesh->state)){
			continue;
		}
		if(!entry.framebuffer.isEquivalent(state.pass.framebuffer->getLayoutState())){
			continue;
		}
		return entry.pipeline;
	}

	// Else we have to create the pipeline
	return createNewPipeline(state, hash);
}

void PipelineCache::freeOutdatedPipelines(){
	GPUContext* context = GPU::getInternal();
	for(VkPipeline& pipeline: _pipelinesToDelete){
		vkDestroyPipeline(context->device, pipeline, nullptr);
	}
	_pipelinesToDelete.clear();
}

void PipelineCache::clean(){
	GPUContext* context = GPU::getInternal();
	freeOutdatedPipelines();
	// Retrieve cache data.
	size_t pipelineSize = 0;
	vkGetPipelineCacheData(context->device, _vulkanCache, &pipelineSize, nullptr);
	if(pipelineSize != 0){
		char* pipelineData = new char[pipelineSize];
		vkGetPipelineCacheData(context->device, _vulkanCache, &pipelineSize, pipelineData);
		Resources::saveRawDataToExternalFile(PIPELINE_CACHE_FILE, pipelineData, pipelineSize);
	}

	for(auto& programPipelines : _pipelines){
		for(auto& pipeline : programPipelines.second){
			vkDestroyPipeline(context->device, pipeline.second.pipeline, nullptr);
		}
	}

	vkDestroyPipelineCache(context->device, _vulkanCache, nullptr);

}

VkPipeline PipelineCache::createNewPipeline(const GPUState& state, const uint64_t hash){
	Entry entry;
	entry.pipeline = buildPipeline(state);
	entry.program = state.program;
	entry.mesh = state.mesh->state;
	entry.framebuffer = state.pass.framebuffer->getLayoutState();

	auto it = _pipelines[state.program].insert(std::make_pair(hash, entry));
	return it->second.pipeline;
}

VkPipeline PipelineCache::buildPipeline(const GPUState& state){
	GPUContext* context = GPU::getInternal();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// Assert no null data.
	assert(state.program); assert(state.mesh); assert(state.pass.framebuffer);
	
	// Program
	{
		const Program::StagesState& programState = state.program->getState();
		pipelineInfo.stageCount = programState.stages.size();
		pipelineInfo.pStages = programState.stages.data();
		pipelineInfo.layout = programState.layout;
	}
	
	// Vertex input.
	VkPipelineVertexInputStateCreateInfo vertexState{};
	{
		const GPUMesh::InputState& meshState = state.mesh->state;
		vertexState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexState.vertexBindingDescriptionCount = meshState.bindings.size();
		vertexState.pVertexBindingDescriptions = meshState.bindings.data();
		vertexState.vertexAttributeDescriptionCount = meshState.attributes.size();
		vertexState.pVertexAttributeDescriptions = meshState.attributes.data();
		pipelineInfo.pVertexInputState = &vertexState;
	}
	// Input assembly.
	VkPipelineInputAssemblyStateCreateInfo assemblyState{};
	{
		assemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assemblyState.primitiveRestartEnable = VK_FALSE;
		assemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineInfo.pInputAssemblyState  = &assemblyState;
	}
	// Viewport (will be dynamic)
	VkPipelineViewportStateCreateInfo viewportState{};
	{
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.scissorCount = 1;
		viewportState.viewportCount = 1;
		viewportState.pScissors = nullptr;
		viewportState.pViewports = nullptr;
		pipelineInfo.pViewportState = &viewportState;
	}
	// Rasterization
	VkPipelineRasterizationStateCreateInfo rasterizationState{};
	{
		static const std::map<PolygonMode, VkPolygonMode> polygon = {
					{PolygonMode::FILL, VK_POLYGON_MODE_FILL},
					{PolygonMode::LINE, VK_POLYGON_MODE_LINE},
					{PolygonMode::POINT, VK_POLYGON_MODE_POINT}};
		static const std::map<Faces, VkCullModeFlags> culling = {
					{Faces::FRONT, VK_CULL_MODE_FRONT_BIT},
					{Faces::BACK, VK_CULL_MODE_BACK_BIT},
					{Faces::ALL, VK_CULL_MODE_FRONT_AND_BACK}};

		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.polygonMode = polygon.at(state.polygonMode);
		rasterizationState.cullMode = state.cullFace ? culling.at(state.cullFaceMode) : VK_CULL_MODE_NONE;

		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.depthBiasClamp = 0.0f;
		rasterizationState.depthBiasEnable = VK_FALSE;
		rasterizationState.depthBiasConstantFactor = 0.0f;
		rasterizationState.depthBiasSlopeFactor = 0.0f;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.lineWidth = 1.f;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		pipelineInfo.pRasterizationState = &rasterizationState;
	}
	// Multisampling (never)
	VkPipelineMultisampleStateCreateInfo msaaState{};
	{
		msaaState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		msaaState.alphaToCoverageEnable = VK_FALSE;
		msaaState.alphaToOneEnable = VK_FALSE;
		msaaState.minSampleShading = 1.f;
		msaaState.pSampleMask = nullptr;
		msaaState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		msaaState.sampleShadingEnable = VK_FALSE;
		pipelineInfo.pMultisampleState = &msaaState;
	}
	// Depth/stencil
	VkPipelineDepthStencilStateCreateInfo depthState{};
	{
		depthState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		static const std::map<TestFunction, VkCompareOp> compare = {
			{TestFunction::NEVER, VK_COMPARE_OP_NEVER},
			{TestFunction::LESS, VK_COMPARE_OP_LESS},
			{TestFunction::LEQUAL, VK_COMPARE_OP_LESS_OR_EQUAL},
			{TestFunction::EQUAL, VK_COMPARE_OP_EQUAL},
			{TestFunction::GREATER, VK_COMPARE_OP_GREATER},
			{TestFunction::GEQUAL, VK_COMPARE_OP_GREATER_OR_EQUAL},
			{TestFunction::NOTEQUAL, VK_COMPARE_OP_NOT_EQUAL},
			{TestFunction::ALWAYS, VK_COMPARE_OP_ALWAYS}};

		depthState.depthCompareOp = compare.at(state.depthFunc);
		depthState.depthTestEnable = state.depthTest;
		depthState.depthWriteEnable = state.depthWriteMask;

		depthState.depthBoundsTestEnable = VK_FALSE;
		depthState.minDepthBounds = 0.0f;
		depthState.maxDepthBounds = 1.0f;

		static const std::map<StencilOp, VkStencilOp> stencil = {
				{ StencilOp::KEEP, VK_STENCIL_OP_KEEP },
				{ StencilOp::ZERO, VK_STENCIL_OP_ZERO },
				{ StencilOp::REPLACE, VK_STENCIL_OP_REPLACE },
				{ StencilOp::INCR, VK_STENCIL_OP_INCREMENT_AND_CLAMP },
				{ StencilOp::INCRWRAP, VK_STENCIL_OP_INCREMENT_AND_WRAP },
				{ StencilOp::DECR, VK_STENCIL_OP_DECREMENT_AND_CLAMP },
				{ StencilOp::DECRWRAP, VK_STENCIL_OP_DECREMENT_AND_WRAP },
				{ StencilOp::INVERT, VK_STENCIL_OP_INVERT }};

		depthState.stencilTestEnable = state.stencilTest;

		depthState.front.writeMask = state.stencilWriteMask ? 0xFF : 0x00;
		depthState.front.compareMask = 0xFF;
		depthState.front.compareOp = compare.at(state.stencilFunc);
		depthState.front.depthFailOp = stencil.at(state.stencilPass);
		depthState.front.failOp = stencil.at(state.stencilFail);
		depthState.front.passOp = stencil.at(state.stencilDepthPass);
		depthState.front.reference = state.stencilValue;
		depthState.back.writeMask = state.stencilWriteMask ? 0xFF : 0x00;
		depthState.back.compareMask = 0xFF;
		depthState.back.compareOp = compare.at(state.stencilFunc);
		depthState.back.depthFailOp = stencil.at(state.stencilPass);
		depthState.back.failOp = stencil.at(state.stencilFail);
		depthState.back.passOp = stencil.at(state.stencilDepthPass);
		depthState.back.reference = state.stencilValue;

		pipelineInfo.pDepthStencilState = &depthState;
	}
	// Color blending
	VkPipelineColorBlendStateCreateInfo colorState{};
	const uint attachmentCount = state.pass.framebuffer->attachments();
	std::vector<VkPipelineColorBlendAttachmentState> attachmentStates(attachmentCount);
	{
		static const std::map<BlendEquation, VkBlendOp> eqs = {
			{BlendEquation::ADD, VK_BLEND_OP_ADD},
			{BlendEquation::SUBTRACT, VK_BLEND_OP_SUBTRACT},
			{BlendEquation::REVERSE_SUBTRACT, VK_BLEND_OP_REVERSE_SUBTRACT},
			{BlendEquation::MIN, VK_BLEND_OP_MIN},
			{BlendEquation::MAX, VK_BLEND_OP_MAX}};
		static const std::map<BlendFunction, VkBlendFactor> funcs = {
			{BlendFunction::ONE, VK_BLEND_FACTOR_ONE},
			{BlendFunction::ZERO, VK_BLEND_FACTOR_ZERO},
			{BlendFunction::SRC_COLOR, VK_BLEND_FACTOR_SRC_COLOR},
			{BlendFunction::ONE_MINUS_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR},
			{BlendFunction::SRC_ALPHA, VK_BLEND_FACTOR_SRC_ALPHA},
			{BlendFunction::ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA},
			{BlendFunction::DST_COLOR, VK_BLEND_FACTOR_DST_COLOR},
			{BlendFunction::ONE_MINUS_DST_COLOR, VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR},
			{BlendFunction::DST_ALPHA, VK_BLEND_FACTOR_DST_ALPHA},
			{BlendFunction::ONE_MINUS_DST_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA}};

		colorState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorState.blendConstants[0] = state.blendColor[0];
		colorState.blendConstants[1] = state.blendColor[1];
		colorState.blendConstants[2] = state.blendColor[2];
		colorState.blendConstants[3] = state.blendColor[3];
		colorState.logicOpEnable = VK_FALSE;
		colorState.logicOp = VK_LOGIC_OP_COPY;
		// Per attachment blending.
		const uint32_t colorMask = (state.colorWriteMask[0] ? VK_COLOR_COMPONENT_R_BIT : 0)
								 | (state.colorWriteMask[1] ? VK_COLOR_COMPONENT_G_BIT : 0)
								 | (state.colorWriteMask[2] ? VK_COLOR_COMPONENT_B_BIT : 0)
								 | (state.colorWriteMask[3] ? VK_COLOR_COMPONENT_A_BIT : 0);
		for(uint aid = 0; aid < attachmentCount; ++aid){
			VkPipelineColorBlendAttachmentState& blendState = attachmentStates[aid];
			blendState.blendEnable = state.blend;
			blendState.alphaBlendOp = eqs.at(state.blendEquationAlpha);
			blendState.colorBlendOp = eqs.at(state.blendEquationRGB);
			blendState.srcColorBlendFactor = funcs.at(state.blendSrcRGB);
			blendState.srcAlphaBlendFactor = funcs.at(state.blendSrcAlpha);
			blendState.dstColorBlendFactor = funcs.at(state.blendDstRGB);
			blendState.dstAlphaBlendFactor = funcs.at(state.blendDstAlpha);
			blendState.colorWriteMask = colorMask;
		}
		colorState.attachmentCount = attachmentCount;
		colorState.pAttachments = attachmentStates.data();

		pipelineInfo.pColorBlendState = &colorState;
	}
	// Dynamic state
	VkPipelineDynamicStateCreateInfo dynamicState{};
	const VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	{
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = &dynStates[0];
		pipelineInfo.pDynamicState = &dynamicState;
	}

	// Render pass
	{
		pipelineInfo.renderPass = state.pass.framebuffer->getRenderPass();
		pipelineInfo.subpass = 0;
	}
	// No inheritance
	{
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;
	}

	VkPipeline pipeline;
	if(vkCreateGraphicsPipelines(context->device, _vulkanCache, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS){
		Log::Error() << Log::GPU << "Unable to create pipeline." << std::endl;
	}
	return pipeline;
}
