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
	TESSCONTROL, ///< Tesselation control shader.
	TESSEVAL, ///< Tesselation evaluation shader.
	COMPUTE, ///< Compute shader.
	COUNT
};

STD_HASH(ShaderType);

/**
\brief The type of data a buffer is storing, determining its use.
\ingroup Resources
*/
enum class BufferType : uint {
	VERTEX, ///< Vertex data.
	INDEX, ///< Element indices.
	UNIFORM, ///< Uniform data.
	CPUTOGPU, ///< Transfer.
	GPUTOCPU, ///< Transfer.
	STORAGE ///< Compute storage.
};

STD_HASH(BufferType);

/**
\brief The frequency at which a uniform buffer might be updated.
\ingroup Resources
*/
enum class UniformFrequency : uint {
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

STD_HASH(TestFunction);

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

STD_HASH(StencilOp);

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

STD_HASH(BlendEquation);


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

STD_HASH(BlendFunction);

/**
\brief Used to select a subset of faces. Front faces are defined counter-clockwise.
\ingroup Resources
*/
enum class Faces : uint {
	FRONT, ///< Front (CCW) faces
	BACK, ///< Back (CW) faces
	ALL ///< All faces
};

STD_HASH(Faces);

/**
\brief How polygons should be rasterized
\ingroup Resources
*/
enum class PolygonMode : uint {
	FILL, ///< As filled polygons.
	LINE, ///< As wireframe edges.
	POINT ///< As vertex points.
};

STD_HASH(PolygonMode);

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

STD_HASH(TextureShape);

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

STD_HASH(Filter);

/**
 \brief The wrapping mode of a texture.
 \ingroup Resources
 */
enum class Wrap : uint {
	CLAMP = 0, ///< Clamp to the edges of the texture.
	REPEAT, ///< Repeat the texture.
	MIRROR ///< Repeat the texture using flipped versions to ensure continuity.
};

STD_HASH(Wrap);

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

STD_HASH(Layout);

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

	/// \brief Current framebuffer information.
	struct FramebufferInfos {
		const Framebuffer* framebuffer = nullptr; ///< The current framebuffer.
		uint mipStart = 0; ///< First mip to be used in the current render pass.
		uint mipCount = 1; ///< Number of mips used in the current render pass.
		uint layerStart = 0; ///< First layer to be used in the current render pass.
		uint layerCount = 1; ///< Number of layers used in the current render pass.
	};

	/// Constructor.
	GPUState() = default;

	/** Test if this state is equivalent (in a Vulkan graphics pipeline state sense) to another.
	 \param other the state to compare to
	 \return true if they are compatible
	 */
	bool isGraphicsEquivalent(const GPUState& other) const;

	/** Test if this state is equivalent (in a Vulkan compute pipeline state sense) to another.
	 \param other the state to compare to
	 \return true if they are compatible
	 */
	bool isComputeEquivalent(const GPUState& other) const;

	// Blend state.
	glm::vec4 blendColor {0.0f}; ///< Blend color for constant blend mode.

	// Color state.
	glm::bvec4 colorWriteMask {true}; ///< Which channels should be written to when rendering.

	// Blend functions.
	BlendFunction blendSrcRGB = BlendFunction::ONE; ///< Blending source type for RGB channels.
	BlendFunction blendSrcAlpha = BlendFunction::ONE; ///< Blending source type for alpha channel.
	BlendFunction blendDstRGB = BlendFunction::ONE; ///< Blending destination type for RGB channels.
	BlendFunction blendDstAlpha = BlendFunction::ONE; ///< Blending destination type for alpha channel.
	BlendEquation blendEquationRGB = BlendEquation::ADD; ///< Blending equation for RGB channels.
	BlendEquation blendEquationAlpha = BlendEquation::ADD; ///< Blending equation for alpha channel.

	// Geometry state.
	Faces cullFaceMode = Faces::BACK; ///< Which faces should be culled.
	PolygonMode polygonMode = PolygonMode::FILL; ///< How should polygons be processed.

	// Depth state.
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
	bool depthTest	= false; ///< Is depth test enabled or not.
	bool depthWriteMask	 = true; ///< Should depth be written to the depth buffer or not.
	bool cullFace	  = false; ///< Is backface culling enabled or not.
	bool blend = false; ///< Blending enabled or not.
	bool sentinel = false; ///< Used to delimit the parameters that can be directly compared in memory.

	// Graphics binding state.
	Program* graphicsProgram = nullptr; ///< The current graphics program.
	const GPUMesh* mesh = nullptr; ///< The current mesh.
	FramebufferInfos pass; ///< The current framebuffer.

	// Compute binding state.
	Program* computeProgram = nullptr; ///< The current compute program.
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

	Type _type = GPUQuery::Type::TIME_ELAPSED; ///< The type of query.
	uint _count = 2; ///< Number of queries used internally (two for duration queries)
	uint _offset = 0; ///< Offset of the first query in the query pools.
	bool _ranThisFrame = false; ///< Has the query been run this frame (else we won't fetch its value).
	bool _running = false; ///< Is a measurement currently taking place.

};

STD_HASH(GPUQuery::Type);

/** \brief Descriptor set allocation.
 \ingroup Graphics
 */
struct DescriptorSet {
	VkDescriptorSet handle = VK_NULL_HANDLE; ///< The native handle.
	uint pool = 0; ///< The pool in which the descriptor set has been allocated.
};
