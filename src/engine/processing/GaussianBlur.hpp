#pragma once

#include "resources/Texture.hpp"
#include "graphics/Program.hpp"
#include "Common.hpp"

/**
 \brief Applies an approximate gaussian blur using a dual filtering approach.
 \details Use a downscaled pyramid approach to approximate a gaussian blur with a large radius. 
 The input texture is downscaled a number of times, using a custom filter as described by Marius Bjørge in the 'Bandwidth-Efficient Rendering' presentation, Siggraph 2015
 (https://community.arm.com/cfs-file/__key/communityserver-blogs-components-weblogfiles/00-00-00-20-66/siggraph2015_2D00_mmg_2D00_marius_2D00_slides.pdf).
 The image is then upscaled again with a second custom filter.
 
 \see GPU::Frag::Blur-dual-filter-down, GPU::Frag::Blur-dual-filter-up
 \ingroup Processing
 */
class GaussianBlur {

public:
	/**
	 Constructor. The depth of the gaussian pyramid will determine the strength of the blur, and the computational cost.
	 \param radius the number of levels in the downscaling pyramid
	 \param downscale work at a lower resolution than the target texture
	 \param name debug name for internal buffers
	 */
	GaussianBlur(uint radius, uint downscale, const std::string & name);

	/**
	 Apply the blurring process to a given texture.
	 \note It is possible to use the same texture as input and output.
	 \param src the ID of the texture to process
	 \param dst the destination texture
	 */
	void process(const Texture& src, Texture & dst);

private:

	/**
	  Handle screen resizing if needed.
	 \param width the new width to use
	 \param height the new height to use
	 */
	void resize(uint width, uint height);

	Program * _blurProgramDown;						///< The downscaling filter.
	Program * _blurProgramUp;						///< The upscaling filter.
	Program * _passthrough;							///< The copy program.
	std::vector<Texture> _levels; 					///< Downscaled pyramid textures.
	uint _downscale = 1;							///< Initial downscaling factor.
};
