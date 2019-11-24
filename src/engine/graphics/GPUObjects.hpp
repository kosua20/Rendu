#pragma once

#include "Common.hpp"

/**
 \brief The shape of a texture: dimensions, layers organisation.
 \ingroup Resources
 */
enum class TextureShape : uint {
	D1		  = 1 << 1,		 ///< 1D texture.
	D2		  = 1 << 2,		 ///< 2D texture.
	D3		  = 1 << 3,		 ///< 3D texture.
	Cube	  = 1 << 4,		 ///< Cubemap texture.
	Array	 = 1 << 5,		 ///< General texture array flag.
	Array1D   = D1 | Array,  ///< 1D texture array.
	Array2D   = D2 | Array,  ///< 2D texture array.
	ArrayCube = Cube | Array ///< Cubemap texture array.
};

/** Combining operator for TextureShape.
 \param t0 first flag
 \param t1 second flag
 \return the combination of both flags.
 */
inline TextureShape operator|(TextureShape t0, TextureShape t1) {
	return static_cast<TextureShape>(static_cast<uint>(t0) | static_cast<uint>(t1));
}

/** Extracting operator for TextureShape.
 \param t0 reference flag
 \param t1 flag to extract
 \return true if t0 'contains' t1
 */
inline bool operator&(TextureShape t0, TextureShape t1) {
	return bool(static_cast<uint>(t0) & static_cast<uint>(t1));
}

/**
 \brief The filtering mode of a texture: we deduce the magnification
 filter from the minification filter for now.
 \ingroup Resources
 */
enum class Filter : uint {
	NEAREST = 0, ///< Nearest neighbour, no mipmap.
	LINEAR, ///< Bilinear, no mipmap.
	NEAREST_NEAREST, ///< Nearest neighbour, closest mipmap.
	LINEAR_NEAREST, ///< Bilinear, closest mipmap.
	NEAREST_LINEAR, ///< Nearest neighbour, linear blend of mipmaps.
	LINEAR_LINEAR ///< Bilinear, linear blend of mipmaps.
};

/**
 \brief The wrapping mode of a texture.
 \ingroup Resources
 */
enum class Wrap : uint {
	CLAMP = 0, ///< Clamp to the edges of the texture.
	REPEAT, ///< Repeat the texture.
	MIRROR ///< Repeat the texture using flipped versions to ensure continuity.
};

/**
 \brief The layout of a texture: components count and type.
 \ingroup Resources
 */
enum class Layout : uint {
	R8,
	RG8,
	RGB8,
	RGBA8,
	SRGB8,
	SRGB8_ALPHA8,
	R16,
	RG16,
	RGBA16,
	R8_SNORM,
	RG8_SNORM,
	RGB8_SNORM,
	RGBA8_SNORM,
	R16_SNORM,
	RG16_SNORM,
	RGB16_SNORM,
	R16F,
	RG16F,
	RGB16F,
	RGBA16F,
	R32F,
	RG32F,
	RGB32F,
	RGBA32F,
	RGB5_A1,
	RGB10_A2,
	R11F_G11F_B10F,
	DEPTH_COMPONENT32F,
	DEPTH24_STENCIL8,
	DEPTH_COMPONENT16,
	DEPTH_COMPONENT24,
	DEPTH32F_STENCIL8,
	R8UI,
	R16I,
	R16UI,
	R32I,
	R32UI,
	RG8I,
	RG8UI,
	RG16I,
	RG16UI,
	RG32I,
	RG32UI,
	RGB8I,
	RGB8UI,
	RGB16I,
	RGB16UI,
	RGB32I,
	RGB32UI,
	RGBA8I,
	RGBA8UI,
	RGBA16I,
	RGBA16UI,
	RGBA32I,
	RGBA32UI
};

/** \brief Regroups format, type, filtering and wrapping informations for a color buffer.
  \ingroup Graphics
 */
class Descriptor {
public:
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

	/** Query the data layout.
	 \return the layout
	 */
	Layout typedFormat() const { return _typedFormat; }

	/** Query the filtering mode.
	 \return the filtering mode
	 */
	Filter filtering() const { return _filtering; }

	/** Query the wrapping mode.
	 \return the wrapping mode
	 */
	Wrap wrapping() const { return _wrapping; }

	/** Obtain the separate GPU type, format, typed format and channel count of the descriptor.
	 \param detailedFormat will contain the type format (GL_RG32F,...)
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

	/** Obtain the GPU texture wrapping mode.
	 \return the wrapping mode
	 */
	GLenum getGPUWrapping() const;

	/** Equality operator.
	 \param other other descriptor to compare to
	 \return true if layout, wrapping and filtering are identical.
	 **/
	bool operator==(const Descriptor & other) const;

private:
	/** Convert a filtering mode to the corresponding GPU driver value.
	 \param filter the filtering mode
	 \return the corresponding driver value
	 */
	static GLenum getGPUFilter(Filter filter);

	Layout _typedFormat; ///< The precise typed format.
	Filter _filtering;   ///< Minification filtering mode.
	Wrap _wrapping;		 ///< Wrapping mode.
};

/**
 \brief Store a texture data on the GPU.
 \ingroup Graphics
 */
class GPUTexture {
public:
	/** Constructor from a layout description and a texture shape.
	 \param texDescriptor the layout descriptor
	 \param shape the texture dimensionality
	 */
	GPUTexture(const Descriptor & texDescriptor, TextureShape shape);

	/** Clean internal GPU buffer. */
	void clean();

	/** Compare the texture layout to another one.
	 \param other the descriptor to compare to
	 \return true if the texture has a similar descriptor
	 */
	bool hasSameLayoutAs(const Descriptor & other) const;

	/** Set the texture filtering.
	 \param filtering the new filtering mode
	 */
	void setFiltering(Filter filtering);

	/** Query the texture descriptor.
	 \return the descriptor used
	 */
	const Descriptor & descriptor() const { return _descriptor; }
	
	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	GPUTexture & operator=(const GPUTexture &) = delete;
	
	/** Copy constructor (disabled). */
	GPUTexture(const GPUTexture &) = delete;
	
	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	GPUTexture & operator=(GPUTexture &&) = delete;
	
	/** Move constructor. */
	GPUTexture(GPUTexture &&) = delete;
	
	// Cached GPU settings.
	const GLenum target;		 ///< Texture target.
	GLenum minFiltering;		 ///< Minification filter.
	GLenum magFiltering;		 ///< Magnification filter.
	GLenum wrapping;			 ///< Wrapping mode.
	const unsigned int channels; ///< Number of channels.
	GLenum typedFormat;			 ///< Detailed format.
	GLenum format;				 ///< General format.
	GLenum type;				 ///< Data type.
	GLuint id = 0; ///< The OpenGL texture ID.
	
private:
	Descriptor _descriptor; ///< Layout used.
};

/**
 \brief Store geometry buffers on the GPU.
 \ingroup Graphics
 */
class GPUMesh {
public:
	GLuint vId	= 0; ///< The vertex array OpenGL ID.
	GLuint eId	= 0; ///< The element buffer OpenGL ID.
	GLsizei count = 0; ///< The number of vertices (cached).
	GLuint vbo	= 0; ///< The vertex buffer objects OpenGL ID.

	/** Clean internal GPU buffers. */
	void clean();
	
	/** Constructor. */
	GPUMesh() = default;
	
	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	GPUMesh & operator=(const GPUMesh &) = delete;
	
	/** Copy constructor (disabled). */
	GPUMesh(const GPUMesh &) = delete;
	
	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	GPUMesh & operator=(GPUMesh &&) = delete;
	
	/** Move constructor. */
	GPUMesh(GPUMesh &&) = delete;
};