#pragma once
#include "resources/Image.hpp"
#include "graphics/GPUObjects.hpp"
#include "Common.hpp"

enum class TextureShape : uint {
	D1 = 1 << 1,
	D2 = 1 << 2,
	D3 = 1 << 3,
	Cube = 1 << 4,
	Array = 1 << 5,
	Array1D = D1 | Array,
	Array2D = D2 | Array,
	ArrayCube = Cube | Array
};
	
inline TextureShape operator |(TextureShape t0, TextureShape t1){
	return static_cast<TextureShape>(static_cast<uint>(t0) | static_cast<uint>(t1));
}

inline bool operator &(TextureShape t0, TextureShape t1){
	return bool(static_cast<uint>(t0) & static_cast<uint>(t1));
}

inline TextureShape& operator |=(TextureShape t0, TextureShape t1){
	return t0 = t0 | t1;
}


/**
 \brief Represents a texture containing one or more images, stored on the CPU and/or GPU.
 \ingroup Resources
 */
class Texture {
	
public:
	
	std::vector<Image> images; ///< The images CPU data (optional).
	std::unique_ptr<GPUTexture> gpu; ///< The GPU data (optional).
	
	unsigned int width = 0; ///< The texture width.
	unsigned int height = 0; ///< The texture height.
	unsigned int levels = 0; ///< The mipmap count.
	
	TextureShape shape = TextureShape::D2; ///< Texure type.
	
	void clearImages();
	
	void clean();
	
	void upload(const Descriptor & descriptor, bool updateMipmaps);
	
};

