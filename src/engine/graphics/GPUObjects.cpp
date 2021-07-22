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

	/*if(!framebuffer || !other.framebuffer){
		return false;
	}*/

	if(!mesh || !other.mesh){
		return false;
	}

	if(std::memcmp(this, &other, offsetof(GPUState, sentinel)) != 0){
		return false;
	}

	// Framebuffer: same attachment count, same layouts (== compatible render passes: format, sample count, )
	/*if(framebuffer->attachments() != other.framebuffer->attachments()){
		return false;
	}*/

	// Mesh: same bindings, same attributes. Offsets and buffers are dynamic.
	if(!(mesh->state.isEquivalent(other.mesh->state))){
		return false;
	}
	return true;
}

GPUTexture::GPUTexture(const Descriptor & texDescriptor, TextureShape shape) :
	_descriptor(texDescriptor), _shape(shape) {

	VkUtils::typesFromShape(shape, type, viewType);
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
	// Update the descriptor.
	//_descriptor  = Descriptor(_descriptor.typedFormat(), filtering, _descriptor.wrapping());
	//minFiltering = _descriptor.getGPUMinificationFilter();
	//magFiltering = _descriptor.getGPUMagnificationFilter();
	// Change the associated sampler.
	//glBindTexture(target, id);
	//GPU::_metrics.textureBindings += 1;
	//glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFiltering);
	//glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFiltering);
	//GPU::restoreTexture(_shape);
}

GPUBuffer::GPUBuffer(BufferType atype, DataUse use){
	// \todo "use" is only used on UBOs for custom size.
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
	static const std::map<GPUQuery::Type, VkQueryType> types = {
		{ Type::TIME_ELAPSED, VK_QUERY_TYPE_TIMESTAMP},
		{ Type::SAMPLES_DRAWN, VK_QUERY_TYPE_OCCLUSION},
		{ Type::ANY_DRAWN, VK_QUERY_TYPE_OCCLUSION},
		{ Type::PRIMITIVES_GENERATED, VK_QUERY_TYPE_PIPELINE_STATISTICS}
	};

	VkQueryPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	poolInfo.queryType = types.at(type);
	poolInfo.queryCount = _count;
	poolInfo.pipelineStatistics = type == Type::PRIMITIVES_GENERATED ? VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT : 0;

	GPUContext* context = GPU::getInternal();
	for(size_t i = 0; i < _pools.size(); ++i){
		if(vkCreateQueryPool(context->device, &poolInfo, nullptr, &_pools[i]) != VK_SUCCESS){
			Log::Error() << Log::GPU << "Unable to create query pool." << std::endl;
		}
	}

	if(_type == Type::SAMPLES_DRAWN){
		_flags = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;
	}
}

void GPUQuery::begin(){
	if(_running){
		Log::Warning() << "A query is already running. Ignoring the restart." << std::endl;
		return;
	}

	GPUContext* context = GPU::getInternal();
	vkCmdResetQueryPool(context->getCurrentCommandBuffer(), _pools[_current], 0, _type == GPUQuery::Type::TIME_ELAPSED ? 2 : 1);
	//glBeginQuery(_internalType, _ids[_current]);
	if(_type == GPUQuery::Type::TIME_ELAPSED){
		vkCmdWriteTimestamp(context->getCurrentCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, _pools[_current], 0);
	} else {
		vkCmdBeginQuery(context->getCurrentCommandBuffer(), _pools[_current], 0, _flags);
	}
	_running = true;
}

void GPUQuery::end(){
	if(!_running){
		Log::Warning() << "No query running currently. Ignoring the stop." << std::endl;
		return;
	}

	GPUContext* context = GPU::getInternal();

	if(_type == GPUQuery::Type::TIME_ELAPSED){
		vkCmdWriteTimestamp(context->getCurrentCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, _pools[_current], 1);
	} else {
		vkCmdEndQuery(context->getCurrentCommandBuffer(), _pools[_current], 0);
	}

	_running = false;
	_current = (_current + 1) % _pools.size();
}

uint64_t GPUQuery::value(){
	if(_running){
		Log::Warning() << "A query is currently running, stopping it first." << std::endl;
		end();
	}
	// We have incremented to the next query index when ending.
	// Furthermore, the previous index was done at the same frame, so low chance of it being ready.
	// So fetch two before, except if we only have one query (will stall).
	// \todo Check if this is still motivated under Vulkan, is there a risk of querying before the pool reset is applied?
	const size_t finished = _pools.size() == 1 ? 0 : ((_current + _pools.size() - 2) % _pools.size());

	GPUContext* context = GPU::getInternal();

	uint64_t data[2] = {0, 0};
	vkGetQueryPoolResults(context->device, _pools[finished], 0, _count, 2 * sizeof(uint64_t), &data[0], sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

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
	_typedFormat(Layout::RGB8), _filtering(Filter::LINEAR_LINEAR), _wrapping(Wrap::CLAMP) {
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
		{Layout::RGB8, { VK_FORMAT_R8G8B8_UNORM, 3 }},
		{Layout::RGBA8, { VK_FORMAT_R8G8B8A8_UNORM, 4 }},
		{Layout::SRGB8, { VK_FORMAT_R8G8B8_SRGB, 3 }},
		{Layout::SRGB8_ALPHA8, { VK_FORMAT_R8G8B8A8_SRGB, 4 }},
		{Layout::R16, { VK_FORMAT_R16_UNORM, 1 }},
		{Layout::RG16, { VK_FORMAT_R16G16_UNORM, 2 }},
		{Layout::RGBA16, { VK_FORMAT_R16G16B16A16_UNORM, 4 }},
		{Layout::R8_SNORM, { VK_FORMAT_R8_SNORM, 1 }},
		{Layout::RG8_SNORM, { VK_FORMAT_R8G8_SNORM, 2 }},
		{Layout::RGB8_SNORM, { VK_FORMAT_R8G8B8_SNORM, 3 }},
		{Layout::RGBA8_SNORM, { VK_FORMAT_R8G8B8A8_SNORM, 4 }},
		{Layout::R16_SNORM, { VK_FORMAT_R16_SNORM, 1 }},
		{Layout::RG16_SNORM, { VK_FORMAT_R16G16_SNORM, 2 }},
		{Layout::RGB16_SNORM, { VK_FORMAT_R16G16B16_SNORM, 3 }},
		{Layout::R16F, { VK_FORMAT_R16_SFLOAT, 1 }},
		{Layout::RG16F, { VK_FORMAT_R16G16_SFLOAT, 2 }},
		{Layout::RGB16F, { VK_FORMAT_R16G16B16_SFLOAT, 3 }},
		{Layout::RGBA16F, { VK_FORMAT_R16G16B16A16_SFLOAT, 4 }},
		{Layout::R32F, { VK_FORMAT_R32_SFLOAT, 1 }},
		{Layout::RG32F, { VK_FORMAT_R32G32_SFLOAT, 2 }},
		{Layout::RGB32F, { VK_FORMAT_R32G32B32_SFLOAT, 3 }},
		{Layout::RGBA32F, { VK_FORMAT_R32G32B32A32_SFLOAT, 4 }},
		{Layout::RGB5_A1, { VK_FORMAT_R5G5B5A1_UNORM_PACK16, 4 }},
		{Layout::RGB10_A2, { VK_FORMAT_A2R10G10B10_UNORM_PACK32, 4 }},
		{Layout::R11F_G11F_B10F, { VK_FORMAT_B10G11R11_UFLOAT_PACK32, 3 }},
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
		{Layout::RGB8I, { VK_FORMAT_R8G8B8_SINT, 3 }},
		{Layout::RGB8UI, { VK_FORMAT_R8G8B8_UINT, 3 }},
		{Layout::RGB16I, { VK_FORMAT_R16G16B16_SINT, 3 }},
		{Layout::RGB16UI, { VK_FORMAT_R16G16B16_UINT, 3 }},
		{Layout::RGB32I, { VK_FORMAT_R32G32B32_SINT, 3 }},
		{Layout::RGB32UI, { VK_FORMAT_R32G32B32_UINT, 3 }},
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
	return _typedFormat == Layout::SRGB8 || _typedFormat == Layout::SRGB8_ALPHA8;
}

std::string Descriptor::string() const {

	#define STRENUM(X) { Layout::X, #X}

	static const std::map<Layout, std::string> strFormats = {
		STRENUM(R8),
		STRENUM(RG8),
		STRENUM(RGB8),
		STRENUM(RGBA8),
		STRENUM(SRGB8),
		STRENUM(SRGB8_ALPHA8),
		STRENUM(R16),
		STRENUM(RG16),
		STRENUM(RGBA16),
		STRENUM(R8_SNORM),
		STRENUM(RG8_SNORM),
		STRENUM(RGB8_SNORM),
		STRENUM(RGBA8_SNORM),
		STRENUM(R16_SNORM),
		STRENUM(RG16_SNORM),
		STRENUM(RGB16_SNORM),
		STRENUM(R16F),
		STRENUM(RG16F),
		STRENUM(RGB16F),
		STRENUM(RGBA16F),
		STRENUM(R32F),
		STRENUM(RG32F),
		STRENUM(RGB32F),
		STRENUM(RGBA32F),
		STRENUM(RGB5_A1),
		STRENUM(RGB10_A2),
		STRENUM(R11F_G11F_B10F),
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
		STRENUM(RGB8I),
		STRENUM(RGB8UI),
		STRENUM(RGB16I),
		STRENUM(RGB16UI),
		STRENUM(RGB32I),
		STRENUM(RGB32UI),
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
