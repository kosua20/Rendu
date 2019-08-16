#pragma once

#include "Common.hpp"


/**
 \brief The shape of a texture: dimensions, layers organisation.
 \ingroup Resources
 */
enum class TextureShape : uint {
	D1 = 1 << 1, ///< 1D texture.
	D2 = 1 << 2, ///< 2D texture.
	D3 = 1 << 3, ///< 3D texture.
	Cube = 1 << 4, ///< Cubemap texture.
	Array = 1 << 5,  ///< General texture array flag.
	Array1D = D1 | Array, ///< 1D texture array.
	Array2D = D2 | Array, ///< 2D texture array.
	ArrayCube = Cube | Array ///< Cubemap texture array.
};

/** Combining operator for TextureShape.
 \param t0 first flag
 \param t1 second flag
 \return the combination of both flags.
 */
inline TextureShape operator |(TextureShape t0, TextureShape t1){
	return static_cast<TextureShape>(static_cast<uint>(t0) | static_cast<uint>(t1));
}

/** Extracting operator for TextureShape.
 \param t0 reference flag
 \param t1 flag to extract
 \return true if t0 'contains' t1
 */
inline bool operator &(TextureShape t0, TextureShape t1){
	return bool(static_cast<uint>(t0) & static_cast<uint>(t1));
}

enum class Filter : uint {
	NEAREST = 0,
	LINEAR,
	NEAREST_NEAREST,
	LINEAR_NEAREST,
	NEAREST_LINEAR,
	LINEAR_LINEAR
};

enum class Wrap : uint {
	CLAMP = 0,
	REPEAT,
	MIRROR
};

enum Layout {
	R8, RG8, RGB8, RGBA8, SRGB8, SRGB8_ALPHA8, R16, RG16, RGBA16, R8_SNORM, RG8_SNORM, RGB8_SNORM, RGBA8_SNORM, R16_SNORM, RG16_SNORM, RGB16_SNORM, R16F, RG16F, RGB16F, RGBA16F, R32F, RG32F, RGB32F, RGBA32F, RGB5_A1, RGB10_A2, R11F_G11F_B10F, DEPTH_COMPONENT16, DEPTH_COMPONENT24, DEPTH_COMPONENT32F, DEPTH24_STENCIL8, DEPTH32F_STENCIL8, R8UI, R16I, R16UI, R32I, R32UI, RG8I, RG8UI, RG16I, RG16UI, RG32I, RG32UI, RGB8I, RGB8UI, RGB16I, RGB16UI, RGB32I, RGB32UI, RGBA8I, RGBA8UI, RGBA16I, RGBA16UI, RGBA32I, RGBA32UI
};

/** \brief Regroups format, type, filtering and wrapping informations for a color buffer.
  \ingroup Graphics
 */
struct Descriptor {
	
	Layout typedFormat() const { return _typedFormat; } ///< The precise typed format.
	Filter filtering() const { return _filtering; } ///< Minification filtering mode.
	Wrap wrapping()const { return _wrapping; } ///< Wrapping mode.
	
	/** Default constructor. RGB8, linear, clamp. */
	Descriptor();
	
	/** Constructor.
	 \param typedFormat the precise typed format to use
	 \param filtering the texture minification filtering (GL_LINEAR_MIPMAP_NEAREST,...) to use
	 \param wrapping the texture wrapping mode (GL_CLAMP_TO_EDGE) to use
	 */
	Descriptor(Layout typedFormat, Filter filtering, Wrap wrapping);
	
	/** Query the number of channels.
	 \return the number of channels
	 */
	unsigned int getChannelsCount() const;
	
	/** Obtain the separate GPU type, format, typed format and channel count of the descriptor.
	 \param detailedFormat will contain the type format
	 \param type will contain the type (GL_FLOAT,...)
	 \param format will contain the general layout (GL_RG,...)
	 \return the number of channels
	 */
	unsigned int getGPULayout(GLenum & detailedFormat, GLenum & type, GLenum & format) const;
	
	/** Obtain the GPU texture magnification filter, removing the mipmaping qualifier.
	 \return the magnification filter
	 */
	GLenum getGPUMagnificationFilter() const;
	
	/** Obtain the GPU texture minification filter.
	 \return the minification filter
	 */
	GLenum getGPUMinificationFilter() const;
	
	GLenum getGPUWrapping() const;
	
	bool operator ==(const Descriptor &other) const;
	
private:
	
	GLenum getGPUFilter(Filter filter) const;
	
	Layout _typedFormat; ///< The precise typed format.
	Filter _filtering; ///< Minification filtering mode.
	Wrap _wrapping; ///< Wrapping mode.
};

/**
 \brief Store a texture data on the GPU.
 \ingroup Graphics
 */
struct GPUTexture {
public:
	/** Constructor from a layout description and a texture shape.
	 \param texDescriptor the layout descriptor
	 \param shape the texture dimensionality
	 */
	GPUTexture(const Descriptor & texDescriptor, TextureShape shape);
	
	/** Clean internal GPU buffer. */
	void clean();
	
	bool hasSameLayoutAs(const Descriptor & other) const;
	
	void setFiltering(Filter filtering);
	
	// Cached GPU settings.
	const GLenum target; ///< Texture target.
	GLenum minFiltering; ///< Minification filter.
	GLenum magFiltering; ///< Magnification filter.
	GLenum wrapping; ///< Wrapping mode.
	const unsigned int channels; ///< Number of channels.
	GLenum typedFormat; ///< Detailed format.
	GLenum format; ///< General format.
	GLenum type; ///< Data type.
	
	GLuint id = 0; ///< The OpenGL texture ID.
	
private:
	Descriptor descriptor; ///< Layout used.
};

/**
 \brief Store geometry buffers on the GPU.
 \ingroup Graphics
 */
struct GPUMesh {
	
	GLuint vId = 0; ///< The vertex array OpenGL ID.
	GLuint eId = 0; ///< The element buffer OpenGL ID.
	GLsizei count = 0; ///< The number of vertices (cached).
	GLuint vbo = 0; ///< The vertex buffer objects OpenGL ID.
	
	/** Clean internal GPU buffers. */
	void clean();
	
};
