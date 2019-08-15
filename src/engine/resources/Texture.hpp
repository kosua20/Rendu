#pragma once
#include "resources/Image.hpp"
#include "graphics/GPUObjects.hpp"
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

/** Combining operator for TextureShape.
 \param t0 first flag
 \param t1 second flag
 \return reference to the first flag after combination with the second flag.
 */
inline TextureShape& operator |=(TextureShape t0, TextureShape t1){
	return t0 = t0 | t1;
}

/**
 \brief Represents a texture containing one or more images, stored on the CPU and/or GPU.
 \ingroup Resources
 */
class Texture {
public:
	
	/** Clear CPU images data. */
	void clearImages();
	
	/** Cleanup all data. */
	void clean();
	
	/** Send to the GPU.
	 \param descriptor the data layout and type to use for the texture
	 \param updateMipmaps generate the mipmaps automatically
	 */
	void upload(const Descriptor & descriptor, bool updateMipmaps);
	
	std::vector<Image> images; ///< The images CPU data (optional).
	std::unique_ptr<GPUTexture> gpu; ///< The GPU data (optional).
	
	unsigned int width = 0; ///< The texture width.
	unsigned int height = 0; ///< The texture height.
	unsigned int depth = 1; ///< The texture depth.
	unsigned int levels = 1; ///< The mipmap count.
	
	TextureShape shape = TextureShape::D2; ///< Texure type.
	
};

