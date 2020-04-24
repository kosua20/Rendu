#pragma once

#include "processing/Blur.hpp"
#include "graphics/Framebuffer.hpp"
#include "Common.hpp"

/**
 \brief Applies a box blur of fixed radius 2. Correspond to uniformly averaging values over a square window.
 \see GPU::Frag::Box_blur_1, GPU::Frag::Box_blur_2, GPU::Frag::Box_blur_3, GPU::Frag::Box_blur_4
 \see GPU::Frag::Box_blur_approx_1, GPU::Frag::Box_blur_approx_2, GPU::Frag::Box_blur_approx_3, GPU::Frag::Box_blur_approx_4
 \ingroup Processing
 */
class BoxBlur : public Blur {

public:
	/**
	 Constructor. Can use either an exhaustive 5x5 box blur (25 samples) or an approximate version with a checkerboard pattern (13 samples).
	 \param shape the shape of the textures to process
	 \param width the internal resolution width
	 \param height the internal resolution height
	 \param depth the internal texture 3rd dimension
	 \param descriptor the framebuffer format and wrapping descriptor
	 \param approximate toggles the approximate box blur
	 */
	BoxBlur(TextureShape shape, unsigned int width, unsigned int height, unsigned int depth, const Descriptor & descriptor, bool approximate);

	/**
	 Apply the blurring process to a given texture.
	 \param texture the ID of the texture to process
	 */
	void process(const Texture * texture) const;

	/**
	 Clean internal resources.
	 */
	void clean() const;

	/**
	  Handle screen resizing if needed.
	 \param width the new width to use
	 \param height the new height to use
	 */
	void resize(unsigned int width, unsigned int height) const;

	/**
	 Clear the final framebuffer texture.
	 */
	void clear() const;

private:
	const Program * _blurProgram;					///< Box blur program
	std::unique_ptr<Framebuffer> _finalFramebuffer; ///< Final framebuffer.
};
