#include "graphics/GPUTypes.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"
#include "graphics/Framebuffer.hpp"
#include <cstring>

bool GPUState::isEquivalent(const GPUState& other) const {
	// Program: pure comparison.
	if(program != other.program){
		return false;
	}
	// If program just reloaded, pipeline layout might have been invalidated.
	if(program->reloaded()){
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

Descriptor::Descriptor() :
	_typedFormat(Layout::RGBA8), _filtering(Filter::LINEAR_LINEAR), _wrapping(Wrap::CLAMP) {
}

Descriptor::Descriptor(Layout typedFormat, Filter filtering, Wrap wrapping) :
	_typedFormat(typedFormat), _filtering(filtering), _wrapping(wrapping) {
}

unsigned int Descriptor::getChannelsCount() const {
	VkFormat format;
	return VkUtils::getGPULayout(_typedFormat, format);
}

bool Descriptor::operator==(const Descriptor & other) const {
	return other._typedFormat == _typedFormat && other._filtering == _filtering && other._wrapping == _wrapping;
}

bool Descriptor::operator!=(const Descriptor & other) const {
	return other._typedFormat != _typedFormat || other._filtering != _filtering || other._wrapping != _wrapping;
}

bool Descriptor::isSRGB() const {
	return _typedFormat == Layout::SRGB8_ALPHA8 || _typedFormat == Layout::SBGR8_ALPHA8;
}

std::string Descriptor::string() const {

	#define STRENUM(X) { Layout::X, #X}

	static const std::unordered_map<Layout, std::string> strFormats = {
		STRENUM(R8),
		STRENUM(RG8),
		STRENUM(RGBA8),
		STRENUM(SRGB8_ALPHA8),
		STRENUM(BGRA8),
		STRENUM(SBGR8_ALPHA8),
		STRENUM(R16),
		STRENUM(RG16),
		STRENUM(RGBA16),
		STRENUM(R8_SNORM),
		STRENUM(RG8_SNORM),
		STRENUM(RGBA8_SNORM),
		STRENUM(R16_SNORM),
		STRENUM(RG16_SNORM),
		STRENUM(R16F),
		STRENUM(RG16F),
		STRENUM(RGBA16F),
		STRENUM(R32F),
		STRENUM(RG32F),
		STRENUM(RGBA32F),
		STRENUM(RGB5_A1),
		STRENUM(A2_BGR10),
		STRENUM(A2_RGB10),
		STRENUM(DEPTH_COMPONENT32F),
		STRENUM(DEPTH24_STENCIL8),
		STRENUM(DEPTH_COMPONENT16),
		STRENUM(DEPTH32F_STENCIL8),
		STRENUM(R8UI),
		STRENUM(R16I),
		STRENUM(R16UI),
		STRENUM(R32I),
		STRENUM(R32UI),
		STRENUM(RG8I),
		STRENUM(RG8UI),
		STRENUM(RG16I),
		STRENUM(RG16UI),
		STRENUM(RG32I),
		STRENUM(RG32UI),
		STRENUM(RGBA8I),
		STRENUM(RGBA8UI),
		STRENUM(RGBA16I),
		STRENUM(RGBA16UI),
		STRENUM(RGBA32I),
		STRENUM(RGBA32UI)
	};

	#undef STRENUM

	static const std::unordered_map<Filter, std::string> strFilters = {
		{Filter::NEAREST, "Nearest no mip"},		
		{Filter::LINEAR, "Linear no mip"},			
		{Filter::NEAREST_NEAREST, "Nearest near. mip"},
		{Filter::LINEAR_NEAREST, "Linear near. mip"},	
		{Filter::NEAREST_LINEAR, "Nearest lin. mip"},	
		{Filter::LINEAR_LINEAR, "Linear lin. mip"}
	};

	static const std::unordered_map<Wrap, std::string> strWraps = {
		{Wrap::CLAMP, "Clamp"},
		{Wrap::REPEAT, "Repeat"},
		{Wrap::MIRROR, "Mirror"}
	};

	return strFormats.at(_typedFormat) + " - " + strFilters.at(_filtering) + " - " + strWraps.at(_wrapping);
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
		vkCmdWriteTimestamp(context->getCurrentCommandBuffer(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pool, _offset);
	} else {
		vkCmdBeginQuery(context->getCurrentCommandBuffer(), pool, _offset, 0);
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
		vkCmdWriteTimestamp(context->getCurrentCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool, _offset + 1u);
	} else {
		vkCmdEndQuery(context->getCurrentCommandBuffer(), pool, _offset);
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
	vkGetQueryPoolResults(context->device, pool, _offset, _count, 2 * sizeof(uint64_t), &data[0], sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

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
