#pragma once

#include "processing/Blur.hpp"
#include "graphics/Framebuffer.hpp"
#include "Common.hpp"

/**
 \brief Applies a box blur of fixed radius 2. Correspond to uniformly averaging values over a square window.
 \ingroup Processing
 */
class BoxBlur {

public:
	/**
	 Constructor. Can use either an exhaustive 5x5 box blur (25 samples) or an approximate version with a checkerboard pattern (13 samples).
	 \param approximate toggles the approximate box blur
	 */
	BoxBlur(bool approximate);

	/**
	 Apply the blurring process to a given texture.
	 \note It is possible to use the same texture as input and output.
	 \param texture the ID of the texture to process
	 \param framebuffer the destination framebuffer
	 */
	void process(const Texture * texture, Framebuffer & framebuffer);

	/**
	 Clean internal resources.
	 */
	void clean() const;

private:

	/**
	  Handle screen resizing if needed.
	 \param width the new width to use
	 \param height the new height to use
	 */
	void resize(unsigned int width, unsigned int height) const;

	const Program * _blur2D;					///< Box blur program
	const Program * _blurArray;					///< Box blur program
	const Program * _blurCube;					///< Box blur program
	std::unique_ptr<Framebuffer> _intermediate; ///< Intermediate target.
};
