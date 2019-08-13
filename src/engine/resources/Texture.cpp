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

void Texture::upload(const Descriptor & descriptor, bool updateMipmaps){
	
	// Check that the descriptor type is valid.
	GLenum format, type;
	descriptor.getTypeAndFormat(type, format);
	/// \todo Move validation inside GLUtilities or Descriptor.
	const bool validType = type == GL_FLOAT || type == GL_UNSIGNED_BYTE;
	const bool validFormat = format == GL_RED || format == GL_RG || format == GL_RGB || format == GL_RGBA;
	if(!validType || !validFormat){
		Log::Error() << "Invalid descriptor for creating texture from file." << std::endl;
		return;
	}
	
	// Create texture.
	GLUtilities::setupTexture(*this, descriptor);
	GLUtilities::uploadTexture(*this);

	// Generate mipmaps pyramid automatically.
	if(updateMipmaps){
		GLUtilities::generateMipMaps(*this);
	}
	
}
