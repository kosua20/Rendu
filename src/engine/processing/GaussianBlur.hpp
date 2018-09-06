#ifndef GaussianBlur_h
#define GaussianBlur_h
#include "../Common.hpp"
#include "Blur.hpp"

/**
 \brief Applies an approximate gaussian blur.
 \details Use a downscaled pyramid approach to approximate a gaussian blur with a large radius. 
 The input texture is downscaled a number of times, and each level is blurred with the same kernel.
 The levels are then combined again in a unique texture, the blurred result.
 It uses a simplified gaussian sampling kernel, as described in Efficient Gaussian blur with linear sampling (http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/).
 \see GLSL::Frag::Blur
 \see GLSL::Frag::Blur_combine_2, GLSL::Frag::Blur_combine_3, GLSL::Frag::Blur_combine_4, GLSL::Frag::Blur_combine_5, GLSL::Frag::Blur_combine_6
 \ingroup Processing
 */
class GaussianBlur : public Blur {

public:

	/**
	 Constructor. The depth of the gaussian pyramid will determine the strength of the blur, and the computational cost.
	 \param width the internal initial resolution width
	 \param height the internal initial resolution height
	 \param depth the number of levels in the downscaling pyramid
	 \param format the OpenGL format of the internal famebuffers
	 \param type the OpenGL type of the internal famebuffers
	 \param preciseFormat the OpenGL precise format of the internal famebuffers
	 */
	GaussianBlur(unsigned int width, unsigned int height, unsigned int depth, GLuint format, GLuint type, GLuint preciseFormat);

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
	
	std::shared_ptr<ProgramInfos> _blurProgram; ///< The approximate gaussian blurring kernel shader.
	std::shared_ptr<Framebuffer> _finalFramebuffer; ///< Final result framebuffer.
	std::shared_ptr<ProgramInfos> _combineProgram;  ///< Program to merge pyramid levels.
	std::vector<std::shared_ptr<Framebuffer>> _frameBuffers; ///< Downscaled pyramid framebuffers.
	std::vector<std::shared_ptr<Framebuffer>> _frameBuffersBlur; ///< Blurred pyramid framebuffers.
	std::vector<GLuint> _textures; ///< Downscaled pyramid framebuffers.
	
};

#endif
