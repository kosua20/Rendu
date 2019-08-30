#include "graphics/GPUObjects.hpp"
#include "graphics/GLUtilities.hpp"

void GPUMesh::clean() {
	glDeleteBuffers(1, &eId);
	glDeleteVertexArrays(1, &vId);
	glDeleteBuffers(1, &vbo);
	count = eId = vId = vbo = 0;
}

GPUTexture::GPUTexture(const Descriptor & texDescriptor, TextureShape shape) :
	target(GLUtilities::targetFromShape(shape)),
	minFiltering(texDescriptor.getGPUMinificationFilter()),
	magFiltering(texDescriptor.getGPUMagnificationFilter()),
	wrapping(texDescriptor.getGPUWrapping()),
	channels(texDescriptor.getGPULayout(typedFormat, type, format)),
	_descriptor(texDescriptor) {
	texDescriptor.getGPULayout(typedFormat, type, format);
}

void GPUTexture::clean() {
	glDeleteTextures(1, &id);
	id = 0;
}

bool GPUTexture::hasSameLayoutAs(const Descriptor & other) const {
	return _descriptor == other;
}

void GPUTexture::setFiltering(Filter filtering) {
	// Update the descriptor.
	_descriptor  = Descriptor(_descriptor.typedFormat(), filtering, _descriptor.wrapping());
	minFiltering = _descriptor.getGPUMinificationFilter();
	magFiltering = _descriptor.getGPUMagnificationFilter();

	glBindTexture(target, id);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFiltering);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFiltering);
	glBindTexture(target, 0);
}

Descriptor::Descriptor() :
	_typedFormat(Layout::RGB8), _filtering(Filter::LINEAR_LINEAR), _wrapping(Wrap::CLAMP) {
}

Descriptor::Descriptor(Layout typedFormat, Filter filtering, Wrap wrapping) :
	_typedFormat(typedFormat), _filtering(filtering), _wrapping(wrapping) {
}

unsigned int Descriptor::getGPULayout(GLenum & detailedFormat, GLenum & type, GLenum & format) const {

	struct GPULayout {
		GLenum detailedFormat;
		GLenum format;
		GLenum type;
	};

	static const std::map<Layout, GPULayout> formatInfos = {
		{Layout::R8, {GL_R8, GL_RED, GL_UNSIGNED_BYTE}},
		{Layout::RG8, {GL_RG8, GL_RG, GL_UNSIGNED_BYTE}},
		{Layout::RGB8, {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE}},
		{Layout::RGBA8, {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE}},
		{Layout::SRGB8, {GL_SRGB8, GL_RGB, GL_UNSIGNED_BYTE}},
		{Layout::SRGB8_ALPHA8, {GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE}},
		{Layout::R16, {GL_R16, GL_RED, GL_UNSIGNED_SHORT}},
		{Layout::RG16, {GL_RG16, GL_RG, GL_UNSIGNED_SHORT}},
		{Layout::RGBA16, {GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT}},
		{Layout::R8_SNORM, {GL_R8_SNORM, GL_RED, GL_BYTE}},
		{Layout::RG8_SNORM, {GL_RG8_SNORM, GL_RG, GL_BYTE}},
		{Layout::RGB8_SNORM, {GL_RGB8_SNORM, GL_RGB, GL_BYTE}},
		{Layout::RGBA8_SNORM, {GL_RGBA8_SNORM, GL_RGBA, GL_BYTE}},
		{Layout::R16_SNORM, {GL_R16_SNORM, GL_RED, GL_SHORT}},
		{Layout::RG16_SNORM, {GL_RG16_SNORM, GL_RG, GL_SHORT}},
		{Layout::RGB16_SNORM, {GL_RGB16_SNORM, GL_RGB, GL_SHORT}},
		{Layout::R16F, {GL_R16F, GL_RED, GL_HALF_FLOAT}},
		{Layout::RG16F, {GL_RG16F, GL_RG, GL_HALF_FLOAT}},
		{Layout::RGB16F, {GL_RGB16F, GL_RGB, GL_HALF_FLOAT}},
		{Layout::RGBA16F, {GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT}},
		{Layout::R32F, {GL_R32F, GL_RED, GL_FLOAT}},
		{Layout::RG32F, {GL_RG32F, GL_RG, GL_FLOAT}},
		{Layout::RGB32F, {GL_RGB32F, GL_RGB, GL_FLOAT}},
		{Layout::RGBA32F, {GL_RGBA32F, GL_RGBA, GL_FLOAT}},
		{Layout::RGB5_A1, {GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1}},
		{Layout::RGB10_A2, {GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV}},
		{Layout::R11F_G11F_B10F, {GL_R11F_G11F_B10F, GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV}},
		{Layout::DEPTH_COMPONENT16, {GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT}},
		{Layout::DEPTH_COMPONENT24, {GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT}},
		{Layout::DEPTH_COMPONENT32F, {GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT}},
		{Layout::DEPTH24_STENCIL8, {GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8}},
		{Layout::DEPTH32F_STENCIL8, {GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV}},
		{Layout::R8UI, {GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE}},
		{Layout::R16I, {GL_R16I, GL_RED_INTEGER, GL_SHORT}},
		{Layout::R16UI, {GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT}},
		{Layout::R32I, {GL_R32I, GL_RED_INTEGER, GL_INT}},
		{Layout::R32UI, {GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT}},
		{Layout::RG8I, {GL_RG8I, GL_RG_INTEGER, GL_BYTE}},
		{Layout::RG8UI, {GL_RG8UI, GL_RG_INTEGER, GL_UNSIGNED_BYTE}},
		{Layout::RG16I, {GL_RG16I, GL_RG_INTEGER, GL_SHORT}},
		{Layout::RG16UI, {GL_RG16UI, GL_RG_INTEGER, GL_UNSIGNED_SHORT}},
		{Layout::RG32I, {GL_RG32I, GL_RG_INTEGER, GL_INT}},
		{Layout::RG32UI, {GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT}},
		{Layout::RGB8I, {GL_RGB8I, GL_RGB_INTEGER, GL_BYTE}},
		{Layout::RGB8UI, {GL_RGB8UI, GL_RGB_INTEGER, GL_UNSIGNED_BYTE}},
		{Layout::RGB16I, {GL_RGB16I, GL_RGB_INTEGER, GL_SHORT}},
		{Layout::RGB16UI, {GL_RGB16UI, GL_RGB_INTEGER, GL_UNSIGNED_SHORT}},
		{Layout::RGB32I, {GL_RGB32I, GL_RGB_INTEGER, GL_INT}},
		{Layout::RGB32UI, {GL_RGB32UI, GL_RGB_INTEGER, GL_UNSIGNED_INT}},
		{Layout::RGBA8I, {GL_RGBA8I, GL_RGBA_INTEGER, GL_BYTE}},
		{Layout::RGBA8UI, {GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE}},
		{Layout::RGBA16I, {GL_RGBA16I, GL_RGBA_INTEGER, GL_SHORT}},
		{Layout::RGBA16UI, {GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT}},
		{Layout::RGBA32I, {GL_RGBA32I, GL_RGBA_INTEGER, GL_INT}},
		{Layout::RGBA32UI, {GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT}}};

	if(formatInfos.count(_typedFormat) > 0) {
		const auto & infos	= formatInfos.at(_typedFormat);
		detailedFormat		  = infos.detailedFormat;
		type				  = infos.type;
		format				  = infos.format;
		const bool oneChannel = (format == GL_RED || format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL);
		return (oneChannel ? 1 : (format == GL_RG ? 2 : (format == GL_RGB ? 3 : 4)));
	}

	Log::Error() << Log::OpenGL << "Unable to find type and format (typed format " << uint(_typedFormat) << ")." << std::endl;
	return 0;
}

GLenum Descriptor::getGPUFilter(Filter filter) {
	static const std::map<Filter, GLenum> filters = {
		{Filter::NEAREST, GL_NEAREST},
		{Filter::LINEAR, GL_LINEAR},
		{Filter::NEAREST_NEAREST, GL_NEAREST_MIPMAP_NEAREST},
		{Filter::LINEAR_NEAREST, GL_LINEAR_MIPMAP_NEAREST},
		{Filter::NEAREST_LINEAR, GL_NEAREST_MIPMAP_LINEAR},
		{Filter::LINEAR_LINEAR, GL_LINEAR_MIPMAP_LINEAR}};
	return filters.at(filter);
}

GLenum Descriptor::getGPUMagnificationFilter() const {
	if(_filtering == Filter::NEAREST_NEAREST || _filtering == Filter::NEAREST_LINEAR) {
		return getGPUFilter(Filter::NEAREST);
	}
	if(_filtering == Filter::LINEAR_NEAREST || _filtering == Filter::LINEAR_LINEAR) {
		return getGPUFilter(Filter::LINEAR);
	}
	return getGPUFilter(_filtering);
}

unsigned int Descriptor::getChannelsCount() const {
	GLenum typeFormat, format, type;
	return getGPULayout(typeFormat, type, format);
}

GLenum Descriptor::getGPUMinificationFilter() const {
	return getGPUFilter(_filtering);
}

GLenum Descriptor::getGPUWrapping() const {
	static const std::map<Wrap, GLenum> wraps = {
		{Wrap::CLAMP, GL_CLAMP_TO_EDGE},
		{Wrap::REPEAT, GL_REPEAT},
		{Wrap::MIRROR, GL_MIRRORED_REPEAT}};
	return wraps.at(_wrapping);
}

bool Descriptor::operator==(const Descriptor & other) const {
	return other._typedFormat == _typedFormat && other._filtering == _filtering && other._wrapping == _wrapping;
}
