#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"

#include <map>

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

GPUTexture::GPUTexture(const Descriptor & texDescriptor) :
	_descriptor(texDescriptor) {

	channels = texDescriptor.getGPULayout(format);
	wrapping = texDescriptor.getGPUWrapping();
	texDescriptor.getGPUFilter(imgFiltering, mipFiltering);

	const Layout layout = texDescriptor.typedFormat();
	const bool isDepth = layout == Layout::DEPTH_COMPONENT16 || layout == Layout::DEPTH_COMPONENT24 || layout == Layout::DEPTH_COMPONENT32F || layout == Layout::DEPTH24_STENCIL8 || layout == Layout::DEPTH32F_STENCIL8;
	const bool isStencil = layout == Layout::DEPTH24_STENCIL8 || layout == Layout::DEPTH32F_STENCIL8;

	aspect = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	if(isStencil){
		aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
}

void GPUTexture::clean() {
	GPU::clean(*this);
}

bool GPUTexture::hasSameLayoutAs(const Descriptor & other) const {
	return _descriptor == other;
}

void GPUTexture::setFiltering(Filter filtering) {
	(void)filtering;
	_descriptor  = Descriptor(_descriptor.typedFormat(), filtering, _descriptor.wrapping());
	// Update sampler parameters.
	wrapping = _descriptor.getGPUWrapping();
	_descriptor.getGPUFilter(imgFiltering, mipFiltering);
	// Recreate sampler.
	GPU::setupSampler(*this);
}

GPUBuffer::GPUBuffer(BufferType atype){
	mappable = (atype == BufferType::UNIFORM || atype == BufferType::CPUTOGPU || atype == BufferType::GPUTOCPU);
}

void GPUBuffer::clean(){
	GPU::clean(*this);
}

void GPUMesh::clean() {
	if(vertexBuffer){
		vertexBuffer->clean();
	}
	if(indexBuffer){
		indexBuffer->clean();
	}
	vertexBuffer.reset();
	indexBuffer.reset();
	count = 0;
	
	GPU::clean(*this);
}

bool GPUMesh::isEquivalent(const GPUMesh& other) const {
	return state.isEquivalent(other.state);
}

bool GPUMesh::InputState::isEquivalent(const GPUMesh::InputState& other) const {
	if(bindings.size() != other.bindings.size()){
		return false;
	}
	if(attributes.size() != other.attributes.size()){
		return false;
	}
	const size_t bindingCount = bindings.size();
	const size_t attributeCount = attributes.size();

	for(size_t bid = 0; bid < bindingCount; ++bid){
		const auto& bind = bindings[bid];
		const auto& obind = other.bindings[bid];
		if((bind.binding != obind.binding) ||
			(bind.stride != obind.stride) ||
			(bind.inputRate != obind.inputRate)){
			return false;
		}
	}

	for(size_t aid = 0; aid < attributeCount; ++aid){
		const auto& attr = attributes[aid];
		const auto& ottr = other.attributes[aid];
		if((attr.binding != ottr.binding) ||
		   (attr.format != ottr.format) ||
		   (attr.location != ottr.location) ||
		   (attr.offset != ottr.offset)){
			return false;
		}
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
	VkQueryPool& pool = context->queryAllocators.at(_type).getCurrentPool();
	if(_type == GPUQuery::Type::TIME_ELAPSED){
		vkCmdWriteTimestamp(context->getCurrentCommandBuffer(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pool, _offset);
	} else {
		vkCmdBeginQuery(context->getCurrentCommandBuffer(), pool, _offset, _flags);
	}
	_running = true;
	_neverRan = false;
}

void GPUQuery::end(){
	if(!_running){
		Log::Warning() << "No query running currently. Ignoring the stop." << std::endl;
		return;
	}

	GPUContext* context = GPU::getInternal();
	VkQueryPool& pool = context->queryAllocators.at(_type).getCurrentPool();
	if(_type == GPUQuery::Type::TIME_ELAPSED){
		vkCmdWriteTimestamp(context->getCurrentCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool, _offset + 1u);
	} else {
		vkCmdEndQuery(context->getCurrentCommandBuffer(), pool, _offset);
	}

	_running = false;
}

uint64_t GPUQuery::value(){
	if(_neverRan){
		return 0;
	}

	if(_running){
		Log::Warning() << "A query is currently running, stopping it first." << std::endl;
		end();
	}

	GPUContext* context = GPU::getInternal();
	
	VkQueryPool& pool = context->queryAllocators.at(_type).getPreviousPool();

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


Descriptor::Descriptor() :
	_typedFormat(Layout::RGBA8), _filtering(Filter::LINEAR_LINEAR), _wrapping(Wrap::CLAMP) {
}

Descriptor::Descriptor(Layout typedFormat, Filter filtering, Wrap wrapping) :
	_typedFormat(typedFormat), _filtering(filtering), _wrapping(wrapping) {
}

unsigned int Descriptor::getGPULayout(VkFormat & format) const {

	struct FormatAndChannels {
		VkFormat format;
		int channels;
	};

	static const std::map<Layout, FormatAndChannels> formatInfos = {
		{Layout::R8, { VK_FORMAT_R8_UNORM, 1 }},
		{Layout::RG8, { VK_FORMAT_R8G8_UNORM, 2 }},
		{Layout::RGBA8, { VK_FORMAT_R8G8B8A8_UNORM, 4 }},
		{Layout::BGRA8, { VK_FORMAT_B8G8R8A8_UNORM, 4 }},
		{Layout::SRGB8_ALPHA8, { VK_FORMAT_R8G8B8A8_SRGB, 4 }},
		{Layout::SBGR8_ALPHA8, { VK_FORMAT_B8G8R8A8_SRGB, 4 }},
		{Layout::R16, { VK_FORMAT_R16_UNORM, 1 }},
		{Layout::RG16, { VK_FORMAT_R16G16_UNORM, 2 }},
		{Layout::RGBA16, { VK_FORMAT_R16G16B16A16_UNORM, 4 }},
		{Layout::R8_SNORM, { VK_FORMAT_R8_SNORM, 1 }},
		{Layout::RG8_SNORM, { VK_FORMAT_R8G8_SNORM, 2 }},
		{Layout::RGBA8_SNORM, { VK_FORMAT_R8G8B8A8_SNORM, 4 }},
		{Layout::R16_SNORM, { VK_FORMAT_R16_SNORM, 1 }},
		{Layout::RG16_SNORM, { VK_FORMAT_R16G16_SNORM, 2 }},
		{Layout::R16F, { VK_FORMAT_R16_SFLOAT, 1 }},
		{Layout::RG16F, { VK_FORMAT_R16G16_SFLOAT, 2 }},
		{Layout::RGBA16F, { VK_FORMAT_R16G16B16A16_SFLOAT, 4 }},
		{Layout::R32F, { VK_FORMAT_R32_SFLOAT, 1 }},
		{Layout::RG32F, { VK_FORMAT_R32G32_SFLOAT, 2 }},
		{Layout::RGBA32F, { VK_FORMAT_R32G32B32A32_SFLOAT, 4 }},
		{Layout::RGB5_A1, { VK_FORMAT_R5G5B5A1_UNORM_PACK16, 4 }},
		{Layout::A2_BGR10, { VK_FORMAT_A2B10G10R10_UNORM_PACK32, 4 }},
		{Layout::A2_RGB10, { VK_FORMAT_A2R10G10B10_UNORM_PACK32, 4 }},
		{Layout::DEPTH_COMPONENT16, { VK_FORMAT_D16_UNORM, 1 }},
		{Layout::DEPTH_COMPONENT32F, { VK_FORMAT_D32_SFLOAT, 1 }},
		{Layout::DEPTH24_STENCIL8, { VK_FORMAT_D24_UNORM_S8_UINT, 1 }},
		{Layout::DEPTH32F_STENCIL8, { VK_FORMAT_D32_SFLOAT_S8_UINT, 1 }},
		{Layout::R8UI, { VK_FORMAT_R8_UINT, 1 }},
		{Layout::R16I, { VK_FORMAT_R16_SINT, 1 }},
		{Layout::R16UI, { VK_FORMAT_R16_UINT, 1 }},
		{Layout::R32I, { VK_FORMAT_R32_SINT, 1 }},
		{Layout::R32UI, { VK_FORMAT_R32_UINT, 1 }},
		{Layout::RG8I, { VK_FORMAT_R8G8_SINT, 2 }},
		{Layout::RG8UI, { VK_FORMAT_R8G8_UINT, 2 }},
		{Layout::RG16I, { VK_FORMAT_R16G16_SINT, 2 }},
		{Layout::RG16UI, { VK_FORMAT_R16G16_UINT, 2 }},
		{Layout::RG32I, { VK_FORMAT_R32G32_SINT, 2 }},
		{Layout::RG32UI, { VK_FORMAT_R32G32_UINT, 2 }},
		{Layout::RGBA8I, { VK_FORMAT_R8G8B8A8_SINT, 4 }},
		{Layout::RGBA8UI, { VK_FORMAT_R8G8B8A8_UINT, 4 }},
		{Layout::RGBA16I, { VK_FORMAT_R16G16B16A16_SINT, 4 }},
		{Layout::RGBA16UI, { VK_FORMAT_R16G16B16A16_UINT, 4 }},
		{Layout::RGBA32I, { VK_FORMAT_R32G32B32A32_SINT, 4 }},
		{Layout::RGBA32UI, { VK_FORMAT_R32G32B32A32_UINT, 4 }}
	};

	if(formatInfos.count(_typedFormat) > 0) {
		const auto & infos	= formatInfos.at(_typedFormat);
		format = infos.format;
		return infos.channels;
	}

	Log::Error() << Log::GPU << "Unable to find type and format (typed format " << uint(_typedFormat) << ")." << std::endl;
	return 0;
}

void Descriptor::getGPUFilter(VkFilter & imgFiltering, VkSamplerMipmapMode & mipFiltering) const {
	struct Filters {
		VkFilter img;
		VkSamplerMipmapMode mip;
	};
	static const std::map<Filter, Filters> filters = {
		{Filter::NEAREST, {VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST}},
		{Filter::LINEAR, {VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST}},
		{Filter::NEAREST_NEAREST, {VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST}},
		{Filter::LINEAR_NEAREST, {VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST}},
		{Filter::NEAREST_LINEAR, {VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_LINEAR}},
		{Filter::LINEAR_LINEAR, {VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR}}};
	const auto & infos = filters.at(_filtering);
	imgFiltering = infos.img;
	mipFiltering = infos.mip;
}

unsigned int Descriptor::getChannelsCount() const {
	VkFormat format;
	return getGPULayout(format);
}

VkSamplerAddressMode Descriptor::getGPUWrapping() const {
	static const std::map<Wrap, VkSamplerAddressMode> wraps = {
		{Wrap::CLAMP, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE},
		{Wrap::REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT},
		{Wrap::MIRROR, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT}};
	return wraps.at(_wrapping);
}

bool Descriptor::operator==(const Descriptor & other) const {
	return other._typedFormat == _typedFormat && other._filtering == _filtering && other._wrapping == _wrapping;
}

bool Descriptor::operator!=(const Descriptor & other) const {
	return other._typedFormat != _typedFormat || other._filtering != _filtering || other._wrapping != _wrapping;
}

bool Descriptor::isSRGB() const {
	return _typedFormat == Layout::SRGB8_ALPHA8;
}

std::string Descriptor::string() const {

	#define STRENUM(X) { Layout::X, #X}

	static const std::map<Layout, std::string> strFormats = {
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

	static const std::map<Filter, std::string> strFilters = {
		{Filter::NEAREST, "Nearest no mip"},		
		{Filter::LINEAR, "Linear no mip"},			
		{Filter::NEAREST_NEAREST, "Nearest near. mip"},
		{Filter::LINEAR_NEAREST, "Linear near. mip"},	
		{Filter::NEAREST_LINEAR, "Nearest lin. mip"},	
		{Filter::LINEAR_LINEAR, "Linear lin. mip"}
	};

	static const std::map<Wrap, std::string> strWraps = {
		{Wrap::CLAMP, "Clamp"},
		{Wrap::REPEAT, "Repeat"},
		{Wrap::MIRROR, "Mirror"}
	};

	return strFormats.at(_typedFormat) + " - " + strFilters.at(_filtering) + " - " + strWraps.at(_wrapping);
}
