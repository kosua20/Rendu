#include "graphics/GPUObjects.hpp"

Descriptor::Descriptor(){
	typedFormat = GL_RGB8; filtering = GL_LINEAR_MIPMAP_LINEAR; wrapping = GL_CLAMP_TO_EDGE;
}

Descriptor::Descriptor(const GLuint typedFormat_, const GLuint filtering_, const GLuint wrapping_){
	typedFormat = typedFormat_; filtering = filtering_; wrapping = wrapping_;
}

void GPUMesh::clean(){
	glDeleteBuffers(1, &eId);
	glDeleteVertexArrays(1, &vId);
	glDeleteBuffers(1, &vbo);
	count = eId = vId = vbo = 0;
}

void GPUTexture::clean(){
	glDeleteTextures(1, &id);
	id = 0;
}

unsigned int Descriptor::getTypeAndFormat(GLuint & type, GLuint & format) const {
	
	struct FormatAndType {
		GLuint format;
		GLuint type;
	};
	
	static const std::map<GLuint, FormatAndType> formatInfos = {
		{ GL_R8, { GL_RED, GL_UNSIGNED_BYTE } },
		{ GL_RG8, { GL_RG, GL_UNSIGNED_BYTE } },
		{ GL_RGB8, { GL_RGB, GL_UNSIGNED_BYTE } },
		{ GL_RGBA8, { GL_RGBA, GL_UNSIGNED_BYTE } },
		{ GL_SRGB8, {GL_RGB, GL_UNSIGNED_BYTE} },
		{ GL_SRGB8_ALPHA8, {GL_RGBA, GL_UNSIGNED_BYTE} },
		{ GL_R16, { GL_RED, GL_UNSIGNED_SHORT } },
		{ GL_RG16, { GL_RG, GL_UNSIGNED_SHORT } },
		{ GL_RGBA16, { GL_RGBA, GL_UNSIGNED_SHORT } },
		{ GL_R8_SNORM, { GL_RED, GL_BYTE } },
		{ GL_RG8_SNORM, { GL_RG, GL_BYTE } },
		{ GL_RGB8_SNORM, { GL_RGB, GL_BYTE } },
		{ GL_RGBA8_SNORM, { GL_RGBA, GL_BYTE } },
		{ GL_R16_SNORM, { GL_RED, GL_SHORT } },
		{ GL_RG16_SNORM, { GL_RG, GL_SHORT } },
		{ GL_RGB16_SNORM, { GL_RGB, GL_SHORT } },
		{ GL_R16F, { GL_RED, GL_HALF_FLOAT } },
		{ GL_RG16F, { GL_RG, GL_HALF_FLOAT } },
		{ GL_RGB16F, { GL_RGB, GL_HALF_FLOAT } },
		{ GL_RGBA16F, { GL_RGBA, GL_HALF_FLOAT } },
		{ GL_R32F, { GL_RED, GL_FLOAT } },
		{ GL_RG32F, { GL_RG, GL_FLOAT } },
		{ GL_RGB32F, { GL_RGB, GL_FLOAT } },
		{ GL_RGBA32F, { GL_RGBA, GL_FLOAT } },
		{ GL_RGB5_A1, { GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1 } },
		{ GL_RGB10_A2, { GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV } },
		{ GL_R11F_G11F_B10F, { GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV } },
		{ GL_DEPTH_COMPONENT16, { GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT } },
		{ GL_DEPTH_COMPONENT24, { GL_DEPTH_COMPONENT, GL_UNSIGNED_INT } },
		{ GL_DEPTH_COMPONENT32F, { GL_DEPTH_COMPONENT, GL_FLOAT } },
		{ GL_DEPTH24_STENCIL8, { GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8 } },
		{ GL_DEPTH32F_STENCIL8, { GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV } },
		{ GL_R8UI, { GL_RED_INTEGER, GL_UNSIGNED_BYTE } },
		{ GL_R16I, { GL_RED_INTEGER, GL_SHORT } },
		{ GL_R16UI, { GL_RED_INTEGER, GL_UNSIGNED_SHORT } },
		{ GL_R32I, { GL_RED_INTEGER, GL_INT } },
		{ GL_R32UI, { GL_RED_INTEGER, GL_UNSIGNED_INT } },
		{ GL_RG8I, { GL_RG_INTEGER, GL_BYTE } },
		{ GL_RG8UI, { GL_RG_INTEGER, GL_UNSIGNED_BYTE } },
		{ GL_RG16I, { GL_RG_INTEGER, GL_SHORT } },
		{ GL_RG16UI, { GL_RG_INTEGER, GL_UNSIGNED_SHORT } },
		{ GL_RG32I, { GL_RG_INTEGER, GL_INT } },
		{ GL_RG32UI, { GL_RG_INTEGER, GL_UNSIGNED_INT } },
		{ GL_RGB8I, { GL_RGB_INTEGER, GL_BYTE } },
		{ GL_RGB8UI, { GL_RGB_INTEGER, GL_UNSIGNED_BYTE } },
		{ GL_RGB16I, { GL_RGB_INTEGER, GL_SHORT } },
		{ GL_RGB16UI, { GL_RGB_INTEGER, GL_UNSIGNED_SHORT } },
		{ GL_RGB32I, { GL_RGB_INTEGER, GL_INT } },
		{ GL_RGB32UI, { GL_RGB_INTEGER, GL_UNSIGNED_INT } },
		{ GL_RGBA8I, { GL_RGBA_INTEGER, GL_BYTE } },
		{ GL_RGBA8UI, { GL_RGBA_INTEGER ,GL_UNSIGNED_BYTE } },
		{ GL_RGBA16I, { GL_RGBA_INTEGER, GL_SHORT } },
		{ GL_RGBA16UI, { GL_RGBA_INTEGER, GL_UNSIGNED_SHORT } },
		{ GL_RGBA32I, { GL_RGBA_INTEGER, GL_INT } },
		{ GL_RGBA32UI, { GL_RGBA_INTEGER, GL_UNSIGNED_INT } }
	};
	
	if(formatInfos.count(typedFormat)>0){
		const auto & infos = formatInfos.at(typedFormat);
		type = infos.type;
		format = infos.format;
		const bool oneChannel = (format == GL_RED || format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL);
		return (oneChannel ? 1 : (format == GL_RG ? 2 : (format == GL_RGB ? 3 : 4)));
	}
	
	Log::Error() << Log::OpenGL << "Unable to find type and format (typed format " << typedFormat << ")." << std::endl;
	return 0;
}

GLuint Descriptor::getMagnificationFilter() const {
	if(filtering == GL_NEAREST_MIPMAP_NEAREST || filtering == GL_NEAREST_MIPMAP_LINEAR){
		return GL_NEAREST;
	}
	if(filtering == GL_LINEAR_MIPMAP_NEAREST || filtering == GL_LINEAR_MIPMAP_LINEAR){
		return GL_LINEAR;
	}
	return filtering;
}
