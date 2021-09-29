#include "graphics/GPUTypes.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"
#include "graphics/Framebuffer.hpp"
#include <cstring>

bool GPUState::isGraphicsEquivalent(const GPUState& other) const {
	// Program: pure comparison.
	if(graphicsProgram != other.graphicsProgram){
		return false;
	}
	// If program just reloaded, pipeline layout might have been invalidated.
	if(graphicsProgram->reloaded()){
		return false;
	}

	if(!pass.framebuffer || !other.pass.framebuffer){
		return false;
	}

	if(!mesh || !other.mesh){
		return false;
	}

	if(std::memcmp(this, &other, offsetof(GPUState, sentinel)) != 0){
		return false;
	}

	// Framebuffer: same attachment count, same layouts (== compatible render passes: format, sample count, )
	if(!(pass.framebuffer->isEquivalent(*other.pass.framebuffer))){
		return false;
	}

	// Mesh: same bindings, same attributes. Offsets and buffers are dynamic.
	if(!(mesh->isEquivalent(*other.mesh))){
		return false;
	}
	return true;
}

bool GPUState::isComputeEquivalent(const GPUState& other) const {
	// Program: pure comparison.
	if(computeProgram != other.computeProgram){
		return false;
	}
	// If program just reloaded, pipeline layout might have been invalidated.
	if(computeProgram->reloaded()){
		return false;
	}
	return true;
}

GPUQuery::GPUQuery(Type type) {
	_type = type;
	_count = type == Type::TIME_ELAPSED ? 2 : 1;

	_offset = GPU::getInternal()->queryAllocators.at(_type).allocate();
}

void GPUQuery::begin(){
	if(_running){
		Log::Warning() << "A query is already running. Ignoring the restart." << std::endl;
		return;
	}

	GPUContext* context = GPU::getInternal();
	VkQueryPool& pool = context->queryAllocators.at(_type).getWritePool();
	if(_type == GPUQuery::Type::TIME_ELAPSED){
		vkCmdWriteTimestamp(context->getRenderCommandBuffer(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pool, _offset);
	} else {
		vkCmdBeginQuery(context->getRenderCommandBuffer(), pool, _offset, 0);
	}
	_running = true;
	_ranThisFrame = true;
}

void GPUQuery::end(){
	if(!_running){
		Log::Warning() << "No query running currently. Ignoring the stop." << std::endl;
		return;
	}

	GPUContext* context = GPU::getInternal();
	VkQueryPool& pool = context->queryAllocators.at(_type).getWritePool();
	if(_type == GPUQuery::Type::TIME_ELAPSED){
		vkCmdWriteTimestamp(context->getRenderCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool, _offset + 1u);
	} else {
		vkCmdEndQuery(context->getRenderCommandBuffer(), pool, _offset);
	}

	_running = false;
}

uint64_t GPUQuery::value(){
	if(!_ranThisFrame){
		return 0;
	}
	_ranThisFrame = false;
	if(_running){
		Log::Warning() << "A query is currently running, stopping it first." << std::endl;
		end();
	}

	GPUContext* context = GPU::getInternal();

	VkQueryPool& pool = context->queryAllocators.at(_type).getReadPool();

	uint64_t data[2] = {0, 0};
	VK_RET(vkGetQueryPoolResults(context->device, pool, _offset, _count, 2 * sizeof(uint64_t), &data[0], sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));

	// For duration elapsed, we compute the time between the two timestamps.
	if(_type == GPUQuery::Type::TIME_ELAPSED){
		// Weird thing happened, ignore.
		if(data[0] > data[1]){
			return 0;
		}
		const double duration = double(data[1] - data[0]);
		return uint64_t(context->timestep * duration);
	}
	// Else we just return the first number.
	return data[0];
}
