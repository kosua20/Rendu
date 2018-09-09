#ifndef BoxBlur_h
#define BoxBlur_h
#include "../Common.hpp"
#include "Blur.hpp"

/**
 \brief Applies a box blur of fixed radius 2. Correspond to uniformly averaging values over a square window.
 \see GLSL::Frag::Box_blur_1, GLSL::Frag::Box_blur_2, GLSL::Frag::Box_blur_3, GLSL::Frag::Box_blur_4
 \see GLSL::Frag::Box_blur_approx_1, GLSL::Frag::Box_blur_approx_2, GLSL::Frag::Box_blur_approx_3, GLSL::Frag::Box_blur_approx_4
 \ingroup Processing
 */
class BoxBlur : public Blur {

public:
	
	/**
	 Constructor. Can use either an exhaustive 5x5 box blur (25 samples) or an approximate version with a checkerboard pattern (13 samples).
	 \param width the internal resolution width
	 \param height the internal resolution height
	 \param approximate toggles the approximate box blur
	 \param descriptor the framebuffer format and wrapping descriptor
	 */
	BoxBlur(unsigned int width, unsigned int height, bool approximate, const Framebuffer::Descriptor & descriptor);

	/**
	 \copydoc Blur::process
	 */
	void process(const GLuint textureId);
	
	/**
	 \copydoc Blur::clean
	 */
	void clean() const;

	/**
	 \copydoc Blur::resize
	 */
	void resize(unsigned int width, unsigned int height);

private:
	
	std::shared_ptr<ProgramInfos> _blurProgram; ///< Box blur program
	std::shared_ptr<Framebuffer> _finalFramebuffer; ///< Final framebuffer.
	
};

#endif
