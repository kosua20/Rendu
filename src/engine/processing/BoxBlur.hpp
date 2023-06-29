#pragma once

#include "resources/Texture.hpp"
#include "graphics/Program.hpp"
#include "Common.hpp"

/**
 \brief Applies a box blur of fixed radius 2. Correspond to uniformly averaging values over a 5x5 square window.
 \details An approximate (checkboard pattern) version doing half as many fetches is available. his blur can be applied to 2D, cubemap, 2D arrays and cubemap arrays textures.
 \ingroup Processing
 */
class BoxBlur {

public:
	/**
	 Constructor. Can use either an exhaustive 5x5 box blur (25 samples) or an approximate version with a checkerboard pattern (13 samples).
	 \param approximate toggles the approximate box blur
	 \param name debug name for internal buffers
	 */
	BoxBlur(bool approximate, const std::string & name);

	/**
	 Apply the blurring process to a given texture. 2D, cubemap and their array versions are supported.
	 \note It is possible to use the same texture as input and output.
	 \param src the ID of the texture to process
	 \param dst the destination texture
	 */
	void process(const Texture & src, Texture & dst);

private:

	/**
	  Handle screen resizing if needed.
	 \param width the new width to use
	 \param height the new height to use
	 */
	void resize(uint width, uint height);

	Program * _blur2D;						///< Box blur program
	Program * _blurArray;					///< Box blur program
	Program * _blurCube;					///< Box blur program
	Program * _blurCubeArray;				///< Box blur program
	Texture _intermediate; 					///< Intermediate texture.
};
