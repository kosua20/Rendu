#pragma once

#include "resources/Image.hpp"
#include "Common.hpp"

#include <vector>

struct GPUTexture;

/**
 \brief Represents a texture containing one or more images, with optional GPU infos.
 \ingroup Resources
 */
class Texture {
	
public:
	
	enum Type : int {
		T1D = 1,
		T2D = 2,
		T3D = 4,
		TCube = 8,
		TArray = 16,
	};
	
	std::vector<Image> images; ///< The image data (optional)
	GPUTexture * gpu = nullptr;
	
	unsigned int width; ///< The texture width.
	unsigned int height; ///< The texture height.
	
	Type type; ///< Texure type.
	
	
};

