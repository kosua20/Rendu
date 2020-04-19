#pragma once
#include "resources/Image.hpp"
#include "graphics/GPUObjects.hpp"
#include "Common.hpp"

/**
 \brief Represents a texture containing one or more images, stored on the CPU and/or GPU.
 \ingroup Resources
 */
class Texture {
public:
	
	/** Constructor. */
	Texture() = default;
	
	/** Constructor.
	 \param name the texture identifier
	 */
	Texture(const std::string & name);

	/** Send to the GPU.
	 \param layout the data layout and type to use for the texture
	 \param updateMipmaps generate the mipmaps automatically
	 */
	void upload(const Descriptor & layout, bool updateMipmaps);

	/** Clear CPU images data. */
	void clearImages();
	
	/** Cleanup all data.
	 \note The dimensions and shape of the texture are preserved.
	 */
	void clean();
		
	/** Bilinearly sample a cubemap in a given direction.
	 \param dir the direction to sample
	 \return the sampled color.
	 */
	glm::vec3 sampleCubemap(const glm::vec3 & dir) const;
	
	/** Get the resource name.
		\return the name.
	 */
	const std::string & name() const;

	/** Compute the maximum possible mipmap level based on the texture type and dimensions.
	 \return the maximum level
	 */
	uint getMaxMipLevel() const;

	/** Copy assignment operator (disabled).
	 \return a reference to the object assigned to
	 */
	Texture & operator=(const Texture &) = delete;
	
	/** Copy constructor (disabled). */
	Texture(const Texture &) = delete;
	
	/** Move assignment operator .
	 \return a reference to the object assigned to
	 */
	Texture & operator=(Texture &&) = default;
	
	/** Move constructor. */
	Texture(Texture &&) = default;
	
	std::vector<Image> images;		 ///< The images CPU data (optional).
	std::unique_ptr<GPUTexture> gpu; ///< The GPU data (optional).
	
	unsigned int width  = 0; ///< The texture width.
	unsigned int height = 0; ///< The texture height.
	unsigned int depth  = 1; ///< The texture depth.
	unsigned int levels = 1; ///< The mipmap count.
	
	TextureShape shape = TextureShape::D2; ///< Texure type.
	
private:
		
	std::string _name; ///< Resource name.
	
};
