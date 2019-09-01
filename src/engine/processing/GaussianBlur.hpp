#pragma once

#include "processing/Blur.hpp"
#include "graphics/Framebuffer.hpp"
#include "Common.hpp"

/**
 \brief Applies an approximate gaussian blur using a dual filtering approach.
 \details Use a downscaled pyramid approach to approximate a gaussian blur with a large radius. 
 The input texture is downscaled a number of times, using a custom filter as described by Marius Bjørge in the 'Bandwidth-Efficient Rendering' presentation, Siggraph 2015
 (https://community.arm.com/cfs-file/__key/communityserver-blogs-components-weblogfiles/00-00-00-20-66/siggraph2015_2D00_mmg_2D00_marius_2D00_slides.pdf).
 The image is then upscaled again with a second custom filtered.
 
 \see GPU::Frag::Blur-dual-filter-down, GPU::Frag::Blur-dual-filter-up
 \ingroup Processing
 */
class GaussianBlur : public Blur {

public:
	/**
	 Constructor. The depth of the gaussian pyramid will determine the strength of the blur, and the computational cost.
	 \param width the internal initial resolution width
	 \param height the internal initial resolution height
	 \param depth the number of levels in the downscaling pyramid
	 \param preciseFormat the precise format of the internal famebuffers
	 */
	GaussianBlur(unsigned int width, unsigned int height, unsigned int depth, Layout preciseFormat);

	/**
	 Apply the blurring process to a given texture.
	 \param textureId the ID of the texture to process
	 */
	void process(const Texture * textureId);

	/**
	 Clean internal resources.
	 */
	void clean() const;

	/**
	  Handle screen resizing if needed.
	 \param width the new width to use
	 \param height the new height to use
	 */
	void resize(unsigned int width, unsigned int height);

private:
	const Program * _blurProgramDown;						 ///< The downscaling filter.
	const Program * _blurProgramUp;							 ///< The upscaling filter.
	std::vector<std::unique_ptr<Framebuffer>> _frameBuffers; ///< Downscaled pyramid framebuffers.
};
