#include "resources/Texture.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/GPUObjects.hpp"
#include "graphics/GLUtilities.hpp"

void Texture::clearImages(){
	images.clear();
}

void Texture::clean(){
	clearImages();
	width = height = levels = 0;
	if(gpu){
		gpu->clean();
	}
}

void Texture::upload(const Descriptor & layout, bool updateMipmaps){
	
	// Create texture.
	GLUtilities::setupTexture(*this, layout);
	GLUtilities::uploadTexture(*this);

	// Generate mipmaps pyramid automatically.
	if(updateMipmaps){
		GLUtilities::generateMipMaps(*this);
	}
	
}
