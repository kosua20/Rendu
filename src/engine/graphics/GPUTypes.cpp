#include "graphics/GPUTypes.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"
#include "system/System.hpp"
#include "renderers/DebugViewer.hpp"

#include <cstring>

bool GPUState::RenderPass::isEquivalent(const RenderPass& other) const {

	// Two attachment references are compatible if they have matching format and sample count.
	// We can ignore: resolve, image layouts, load/store operations.
	if( depthStencil != other.depthStencil ){
		return false;
	}

	// We can safely compare color attachments.
	for(uint cid = 0; cid < colors.size(); ++cid){
		if(colors[cid] != other.colors[cid]){
			return false;
		}
	}
	return true;
}

bool GPUState::isGraphicsEquivalent(const GPUState& other) const {
	// Program: pure comparison.
	if(graphicsProgram != other.graphicsProgram){
		return false;
	}
	// If program just reloaded, pipeline layout might have been invalidated.
	if(graphicsProgram->reloaded()){
		return false;
	}

	if(!mesh || !other.mesh){
		return false;
	}

	if(std::memcmp(this, &other, offsetof(GPUState, sentinel)) != 0){
		return false;
	}

	// No texture outputs.
	if(pass.depthStencil == Layout::NONE && pass.colors[0] == Layout::NONE){
		return false;
	}

	// No texture outputs.
	if(other.pass.depthStencil == Layout::NONE && other.pass.colors[0] == Layout::NONE) {
		return false;
	}

	// Attachments: same count, same layouts (== compatible render passes: format, sample count,...)
	if(!(pass.isEquivalent(other.pass))){
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
	VkResult res = vkGetQueryPoolResults(context->device, pool, _offset, _count, 2 * sizeof(uint64_t), &data[0], sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
	// Don't wait for queries if they are not ready (in case we skip a frame while minmized for instance).
	if(res == VK_NOT_READY) {
		return 0;
	}
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

GPUMarker::GPUMarker(const std::string& label, const glm::vec4& color, GPUMarker::Type type, GPUMarker::Target target) : _type(type), _target(target){
	createMarker(label, color);
}

GPUMarker::GPUMarker(const std::string& label, Type type, Target target) : _type(type), _target(target) {
	const uint32_t hash = System::hash32(label.data(), label.size());
	// Basic hash to color conversion, putting more emphasis on the hue.
	float hue 		= (float)((hash & 0x0000ffff)	   )/65535.f;
	float saturation 	= (float)((hash & 0x00ff0000) >> 16)/ 255.0f;
	float value 		= (float)((hash & 0xff000000) >> 24)/ 255.0f;
	// Use the same ranges as in random color generation (see Random.cpp)
	hue *= 360.0f;
	saturation = saturation * 0.45f + 0.5f;
	value = value * 0.45f + 0.5f;

	const glm::vec3 color = glm::rgbColor(glm::vec3(hue , saturation, value));
	createMarker(label, glm::vec4(color, 1.f));
}

const std::unordered_map<GPUMarker::Target, std::string> debugMarkerCategories = {
	{ GPUMarker::Target::RENDER, "Render"},
	{ GPUMarker::Target::UPLOAD, "Upload"},
};

void GPUMarker::createMarker(const std::string& label, const glm::vec4& color){
	GPUContext* context = GPU::getInternal();
	if(!context->markersEnabled){
		return;
	}

	VkDebugUtilsLabelEXT labelInfo = {};
	labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	labelInfo.pLabelName = label.c_str();
	labelInfo.color[0] = color[0];
	labelInfo.color[1] = color[1];
	labelInfo.color[2] = color[2];
	labelInfo.color[3] = color[3];

	VkCommandBuffer& commandBuffer = _target == Target::RENDER ? context->getRenderCommandBuffer() : context->getUploadCommandBuffer();
	const std::string& categoryName = debugMarkerCategories.at(_target);

	if(_type == Type::INSERT){
		vkCmdInsertDebugUtilsLabelEXT(commandBuffer, &labelInfo);
		DebugViewer::insertMarkerDefault(categoryName, label, color);
	} else {
		vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &labelInfo);
		DebugViewer::pushMarkerDefault(categoryName, label, color);
	}
}

GPUMarker::~GPUMarker(){
	GPUContext* context = GPU::getInternal();
	if(!context->markersEnabled){
		return;
	}
	if(_type == Type::INSERT){
		return;
	}

	VkCommandBuffer& commandBuffer = _target == Target::RENDER ? context->getRenderCommandBuffer() : context->getUploadCommandBuffer();
	vkCmdEndDebugUtilsLabelEXT(commandBuffer);

	DebugViewer::popMarkerDefault(debugMarkerCategories.at(_target));
}
