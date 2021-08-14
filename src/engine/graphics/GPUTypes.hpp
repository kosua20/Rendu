#pragma once

#include "Common.hpp"

using GPUAsyncTask = uint64_t;

#ifndef VK_NULL_HANDLE
#define VK_NULL_HANDLE 0
#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
#endif

VK_DEFINE_HANDLE(VkDescriptorSet)

/**
\brief The type of a shader.
\ingroup Resources
*/
enum class ShaderType : uint {
	VERTEX, ///< Vertex shader.
	FRAGMENT, ///< Fragment shader.
	GEOMETRY, ///< Geometry shader.
	TESSCONTROL, ///< Tesselation control shader.
	TESSEVAL, ///< Tesselation evaluation shader.
	COUNT
};

/** Hash specialization for unordered_map/set */
template <> struct std::hash<ShaderType> {
	std::size_t operator()(const ShaderType& t) const { return static_cast<uint>(t); }
};

/**
\brief The type of data a buffer is storing, determining its use.
\ingroup Resources
*/
enum class BufferType : uint {
	VERTEX, ///< Vertex data.
	INDEX, ///< Element indices.
	UNIFORM, ///< Uniform data.
	CPUTOGPU, ///< Transfer.
	GPUTOCPU ///< Transfer.
};

/** Hash specialization for unordered_map/set */
template <> struct std::hash<BufferType> {
	std::size_t operator()(const BufferType& t) const { return static_cast<uint>(t); }
};

/**
\brief The frequency at which a resources might be updated.
\ingroup Resources
*/
enum class DataUse : uint {
	STATIC, ///< Data won't be updated after upload.
	FRAME,  ///< Data will be updated at most once per frame
	DYNAMIC ///< Data will be updated many times per frame.
};

/**
\brief Depth or stencil test function.
\ingroup Resources
*/
enum class TestFunction : uint {
	NEVER, ///< Fail in all cases
	LESS, ///< Pass if lower
	LEQUAL, ///< Pass if lower or equal
	EQUAL, ///< Pass if equal
	GREATER, ///< Pass if greater
	GEQUAL, ///< Pass if greater or equal
	NOTEQUAL, ///< Pass if different
	ALWAYS ///< Always pass
};

/** Hash specialization for unordered_map/set */
template <> struct std::hash<TestFunction> {
	std::size_t operator()(const TestFunction& t) const { return static_cast<uint>(t); }
};

/**
\brief Stencil operation to perform.
\ingroup Resources
*/
enum class StencilOp : uint {
	KEEP, ///< Keep current value.
	ZERO,	///< Set value to zero.
	REPLACE,	///< Set value to reference.
	INCR,	///< Increment value and clamp.
	INCRWRAP, ///< Increment value and wrap.
	DECR,	///< Decrement value and clamp.
	DECRWRAP, ///< Decrement value and wrap.
	INVERT ///< Invert value bitwise.
};

/** Hash specialization for unordered_map/set */
template <> struct std::hash<StencilOp> {
	std::size_t operator()(const StencilOp& t) const { return static_cast<uint>(t); }
};

/**
\brief Blending mix equation for each component. Below we use src and dst to denote
 the (modulated by the blend functions) values to blend.
 Note that min and max do not apply the modulation factor to each term.
\ingroup Resources
*/
enum class BlendEquation : uint {
	ADD, ///< Perform src + dst
	SUBTRACT, ///< Perform src - dst
	REVERSE_SUBTRACT, ///< Perform dst - src
	MIN, ///< Perform min(src, dst)
	MAX ///< Perform max(src, dst)
};

/** Hash specialization for unordered_map/set */
template <> struct std::hash<BlendEquation> {
	std::size_t operator()(const BlendEquation& t) const { return static_cast<uint>(t); }
};


/**
\brief How the source and destination values to blend are obtained from the pixel data by scaling.
\ingroup Resources
*/
enum class BlendFunction : uint {
	ZERO, ///< Multiply by 0
	ONE, ///< Multiply by 1
	SRC_COLOR, ///< Multiply by src color, component wise
	ONE_MINUS_SRC_COLOR, ///< Multiply by 1-src color, component wise
	DST_COLOR, ///< Multiply by dst color, component wise
	ONE_MINUS_DST_COLOR, ///< Multiply by 1-dst color, component wise
	SRC_ALPHA, ///< Multiply by src scalar alpha
	ONE_MINUS_SRC_ALPHA, ///< Multiply by 1-src scalar alpha
	DST_ALPHA, ///< Multiply by dst scalar alpha
	ONE_MINUS_DST_ALPHA ///< Multiply by 1-dst scalar alpha
};

/** Hash specialization for unordered_map/set */
template <> struct std::hash<BlendFunction> {
	std::size_t operator()(const BlendFunction& t) const { return static_cast<uint>(t); }
};

/**
\brief Used to select a subset of faces. Front faces are defined counter-clockwise.
\ingroup Resources
*/
enum class Faces : uint {
	FRONT, ///< Front (CCW) faces
	BACK, ///< Back (CW) faces
	ALL ///< All faces
};

/** Hash specialization for unordered_map/set */
template <> struct std::hash<Faces> {
	std::size_t operator()(const Faces& t) const { return static_cast<uint>(t); }
};

/**
\brief How polygons should be rasterized
\ingroup Resources
*/
enum class PolygonMode : uint {
	FILL, ///< As filled polygons.
	LINE, ///< As wireframe edges.
	POINT ///< As vertex points.
};

/** Hash specialization for unordered_map/set */
template <> struct std::hash<PolygonMode> {
	std::size_t operator()(const PolygonMode& t) const { return static_cast<uint>(t); }
};

/**
 \brief The shape of a texture: dimensions, layers organisation.
 \ingroup Resources
 */
enum class TextureShape : uint {
	D1		  = 1 << 1,		 ///< 1D texture.
	D2		  = 1 << 2,		 ///< 2D texture.
	D3		  = 1 << 3,		 ///< 3D texture.
	Cube	  = 1 << 4,		 ///< Cubemap texture.
	Array	  = 1 << 5,		 ///< General texture array flag.
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

/** Combining operator for TextureShape.
 \param t0 first flag
 \param t1 second flag
 \return reference to the first flag after combination with the second flag.
 */
inline TextureShape & operator|=(TextureShape & t0, TextureShape & t1) {
	return t0 = t0 | t1;
}

/** Hash specialization for unordered_map/set */
template <> struct std::hash<TextureShape> {
	std::size_t operator()(const TextureShape& t) const { return static_cast<uint>(t); }
};

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

/** Hash specialization for unordered_map/set */
template <> struct std::hash<Filter> {
	std::size_t operator()(const Filter& t) const { return static_cast<uint>(t); }
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

/** Hash specialization for unordered_map/set */
template <> struct std::hash<Wrap> {
	std::size_t operator()(const Wrap& t) const { return static_cast<uint>(t); }
};

/**
 \brief The layout of a texture: components count and type.
 \ingroup Resources
 */
enum class Layout : uint {
	R8,
	RG8,
	RGBA8,
	SRGB8_ALPHA8,
	BGRA8,
	SBGR8_ALPHA8,
	R16,
	RG16,
	RGBA16,
	R8_SNORM,
	RG8_SNORM,
	RGBA8_SNORM,
	R16_SNORM,
	RG16_SNORM,
	R16F,
	RG16F,
	RGBA16F,
	R32F,
	RG32F,
	RGBA32F,
	RGB5_A1,
	A2_BGR10,
	A2_RGB10,
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
	RGBA8I,
	RGBA8UI,
	RGBA16I,
	RGBA16UI,
	RGBA32I,
	RGBA32UI
};

/** Hash specialization for unordered_map/set */
template <> struct std::hash<Layout> {
	std::size_t operator()(const Layout& t) const { return static_cast<uint>(t); }
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

	/** Equality operator.
	 \param other other descriptor to compare to
	 \return true if layout, wrapping and filtering are identical.
	 **/
	bool operator==(const Descriptor & other) const;

	/** Non-equality operator.
	 \param other other descriptor to compare to
	 \return true if layout, wrapping and filtering are different.
	 **/
	bool operator!=(const Descriptor & other) const;

	/** Query if the texture is storing gamma-corrected values.
	 \return the srgb status
	 */
	bool isSRGB() const;

	/** Query a string representation of the descriptor.
		\return a string detailing the descriptor settings
	*/
	std::string string() const;

private:

	Layout _typedFormat; ///< The precise typed format.
	Filter _filtering;   ///< Minification filtering mode.
	Wrap _wrapping;		 ///< Wrapping mode.
};

class Program;
class Framebuffer;
class GPUMesh;

/**
 \brief Internal GPU state ; not all API options are exposed, only these that can be toggled in Rendu.
 \note This is only provided as a read-only state. Modifying attributes won't affect the current GPU state.
 \ingroup Graphics
 */
class GPUState {
public:

	/// Constructor.
	GPUState() = default;

	bool isEquivalent(const GPUState& other) const;

	// Blend state.
	glm::vec4 blendColor {0.0f}; ///< Blend color for constant blend mode.

	// Color state.
	glm::bvec4 colorWriteMask {true}; ///< Which channels should be written to when rendering.

	//
	BlendFunction blendSrcRGB = BlendFunction::ONE; ///< Blending source type for RGB channels.
	BlendFunction blendSrcAlpha = BlendFunction::ONE; ///< Blending source type for alpha channel.
	BlendFunction blendDstRGB = BlendFunction::ONE; ///< Blending destination type for RGB channels.
	BlendFunction blendDstAlpha = BlendFunction::ONE; ///< Blending destination type for alpha channel.
	BlendEquation blendEquationRGB = BlendEquation::ADD; ///< Blending equation for RGB channels.
	BlendEquation blendEquationAlpha = BlendEquation::ADD; ///< Blending equation for alpha channel.

	// Geometry state.
	Faces cullFaceMode = Faces::BACK; ///< Which faces should be culled.
	PolygonMode polygonMode = PolygonMode::FILL; ///< How should polygons be processed.
	//float polygonOffsetFactor = 0.0f; ///< Polygon offset depth scaling.
	//float polygonOffsetUnits = 0.0f; ///< Polygon offset depth shifting.
	//bool polygonOffset	= false; ///< Is polygon offset enabled or not.

	// Depth state.
	//glm::vec2 depthRange {0.0f, 1.f}; ///< Depth value valid range.
	TestFunction depthFunc = TestFunction::LESS; ///< Depth test function.
	// Stencil state
	TestFunction stencilFunc = TestFunction::ALWAYS; ///< Stencil test function.
	StencilOp stencilFail = StencilOp::KEEP; ///< Operation when the stencil test fails.
	StencilOp stencilPass = StencilOp::KEEP; ///< Operation when the stencil test passes but the depth test fails.
	StencilOp stencilDepthPass = StencilOp::KEEP; ///< Operation when the stencil and depth tests passes.
	uint patchSize = 3; ///< Tesselation patch size.
	uchar stencilValue = 0; ///< Stencil reference value.
	bool stencilTest = false; ///< Is the stencil test enabled or not.
	bool stencilWriteMask = true; ///< should stencil be written to the stencil buffer or not.
	//float depthClearValue = 1.0f; ///< Depth for clearing depth buffer.
	bool depthTest	= false; ///< Is depth test enabled or not.
	//bool depthClamp	  = false; ///< Should depth be clamped to the valid range or not.
	bool depthWriteMask	 = true; ///< Should depth be written to the depth buffer or not.
	bool cullFace	  = false; ///< Is backface culling enabled or not.
	bool blend = false; ///< Blending enabled or not.

	// Viewport and scissor state.
	//glm::vec4 viewport {0.0f}; ///< Current viewport region.
	bool sentinel = false;


	// Binding state.
	Program* program = nullptr;
	const GPUMesh* mesh = nullptr;

	struct FramebufferInfos {
		const Framebuffer* framebuffer = nullptr;
		uint mipStart = 0;
		uint mipCount = 1;
		uint layerStart = 0;
		uint layerCount = 1;
	};

	FramebufferInfos pass;
};

/** \brief Represent a GPU query, automatically buffered and retrieved.
 \warning You cannot have multiple queries of the same type running at the same time.
 \ingroup Graphics
 */
class GPUQuery {
public:

	/** Type for query to perform. */
	enum class Type : uint {
		TIME_ELAPSED, ///< Time taken by GPU operations between the beginning and end of the query.
		SAMPLES_DRAWN, ///< Number of samples passing the tests while the query is active.
		//PRIMITIVES_GENERATED, unsupported on MVK ///< Number of primitives generated by tesselation or geometry shaders.
		ANY_DRAWN ///< Was any sample drawn while the query was active.
	};

	/** Constructor.
	 \param type the metric to query
	 */
	GPUQuery(Type type = GPUQuery::Type::TIME_ELAPSED);

	/** Start measuring the metric. */
	void begin();

	/** End the measurement. */
	void end();

	/** Query the metric measured at the last frame.
	 Unit used is nanoseconds for timing queries, number of samples for occlusion queries.
	 \return the raw metric value
	 */
	uint64_t value();

private:

	Type _type = GPUQuery::Type::TIME_ELAPSED;
	uint _count = 2;
	uint _offset = 0;
	bool _ranThisFrame = false;
	bool _running = false; ///< Is a measurement currently taking place.

};

/** Hash specialization for unordered_map/set */
template <> struct std::hash<GPUQuery::Type> {
	std::size_t operator()(const GPUQuery::Type& t) const { return static_cast<uint>(t); }
};

struct DescriptorSet {
	VkDescriptorSet handle = VK_NULL_HANDLE;
	uint pool = 0;
};