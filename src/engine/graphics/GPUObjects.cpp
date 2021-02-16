#include "graphics/GPUObjects.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"

#include <map>

GPUTexture::GPUTexture(const Descriptor & texDescriptor, TextureShape shape) :
	_descriptor(texDescriptor), _shape(shape) {

	VkUtils::typesFromShape(shape, type, viewType);
	channels = texDescriptor.getGPULayout(format);
	wrapping = texDescriptor.getGPUWrapping();
	texDescriptor.getGPUFilter(imgFiltering, mipFiltering);
}

void GPUTexture::clean() {
	//glDeleteTextures(1, &id);
	//GPU::deleted(*this);
	//id = 0;
}

bool GPUTexture::hasSameLayoutAs(const Descriptor & other) const {
	return _descriptor == other;
}

void GPUTexture::setFiltering(Filter filtering) {
	// Update the descriptor.
	//_descriptor  = Descriptor(_descriptor.typedFormat(), filtering, _descriptor.wrapping());
	//minFiltering = _descriptor.getGPUMinificationFilter();
	//magFiltering = _descriptor.getGPUMagnificationFilter();

	//glBindTexture(target, id);
	//GPU::_metrics.textureBindings += 1;
	//glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFiltering);
	//glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFiltering);
	//GPU::restoreTexture(_shape);
}

GPUBuffer::GPUBuffer(BufferType type, DataUse use){
//	static const std::map<BufferType, GLenum> types = {
//	{BufferType::VERTEX, GL_ARRAY_BUFFER},
//	{BufferType::INDEX, GL_ELEMENT_ARRAY_BUFFER},
//	{BufferType::UNIFORM, GL_UNIFORM_BUFFER}};
//	target = types.at(type);
//
//	static const std::map<DataUse, GLenum> usages = {
//	{DataUse::STATIC, GL_STATIC_DRAW},
//	{DataUse::DYNAMIC, GL_DYNAMIC_DRAW}};
//	usage = usages.at(use);
}

void GPUBuffer::clean(){
	//glDeleteBuffers(1, &id);
	//id = 0;
}

void GPUMesh::clean() {
//	if(vertexBuffer){
//		vertexBuffer->clean();
//	}
//	if(indexBuffer){
//		indexBuffer->clean();
//	}
//	//glDeleteVertexArrays(1, &id);
//	GPU::deleted(*this);
//	vertexBuffer.reset();
//	indexBuffer.reset();
//	count = 0;
	//id = 0;
}

GPUQuery::GPUQuery(Type type) {
//	static const std::map<GPUQuery::Type, GLenum> types = {
//		{ Type::TIME_ELAPSED, GL_TIME_ELAPSED},
//		{ Type::SAMPLES_DRAWN, GL_SAMPLES_PASSED},
//		{ Type::ANY_DRAWN, GL_ANY_SAMPLES_PASSED},
//		{ Type::PRIMITIVES_GENERATED, GL_PRIMITIVES_GENERATED}
//	};
	//_internalType = types.at(type);
	//glGenQueries(GLsizei(_ids.size()), &_ids[0]);
	// Do dummy initial queries.
	//for(int i = 0; i < 2; ++i){
	//	begin();
	//	end();
	//}
}

void GPUQuery::begin(){
	//if(_running){
		//	Log::Warning() << "A query is already running. Ignoring the restart." << std::endl;
		//	return;
		//}
	//glBeginQuery(_internalType, _ids[_current]);
	//_running = true;
}

void GPUQuery::end(){
	//if(!_running){
	//	Log::Warning() << "No query running currently. Ignoring the stop." << std::endl;
	//	return;
	//}
	//glEndQuery(_internalType);
	//_running = false;
	//_current = (_current + 1) % _ids.size();
}

uint64_t GPUQuery::value(){
	//if(_running){
	//	Log::Warning() << "A query is currently running, stopping it first." << std::endl;
	//	end();
	//}
	// We have incremented to the next query index when ending.
	// Furthermore, the previous index was done at the same frame, so low chance of it being ready.
	// So fetch two before, except if we only have one query (will stall).
	//const size_t finished = _ids.size() == 1 ? 0 : ((_current + _ids.size() - 2) % _ids.size());
	//GLuint64 data = 0;
	//glGetQueryObjectui64v(_ids[finished], GL_QUERY_RESULT, &data);
	return 0;
	//return uint64_t(data);
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


GPUState::GPUState(){
//	for(auto & texbind : textures){
//		texbind[GL_TEXTURE_1D] = 0;
//		texbind[GL_TEXTURE_2D] = 0;
//		texbind[GL_TEXTURE_3D] = 0;
//		texbind[GL_TEXTURE_CUBE_MAP] = 0;
//		texbind[GL_TEXTURE_1D_ARRAY] = 0;
//		texbind[GL_TEXTURE_2D_ARRAY] = 0;
//		texbind[GL_TEXTURE_CUBE_MAP_ARRAY] = 0;
//	}
}
