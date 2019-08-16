#pragma once
#include "resources/Image.hpp"
#include "graphics/GPUObjects.hpp"
#include "Common.hpp"


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
	 \param layout the data layout and type to use for the texture
	 \param updateMipmaps generate the mipmaps automatically
	 */
	void upload(const Descriptor & layout, bool updateMipmaps);
	
	std::vector<Image> images; ///< The images CPU data (optional).
	std::unique_ptr<GPUTexture> gpu; ///< The GPU data (optional).
	
	unsigned int width = 0; ///< The texture width.
	unsigned int height = 0; ///< The texture height.
	unsigned int depth = 1; ///< The texture depth.
	unsigned int levels = 1; ///< The mipmap count.
	
	TextureShape shape = TextureShape::D2; ///< Texure type.
};

