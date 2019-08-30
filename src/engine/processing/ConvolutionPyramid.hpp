#pragma once
#include "graphics/Framebuffer.hpp"
#include "resources/ResourcesManager.hpp"
#include "Common.hpp"

/**
 \brief Implements a multiscale scheme for approximating convolution with large filters.
 This is the basis of the technique described in Convolution Pyramids, Farbman et al., 2011.
 A set of filter parameters can be estimated through an offline optimization for each desired task:
 gradient field integration, seamless image cloning, background filling, or scattered data interpolation.
 \ingroup Processing
 */
class ConvolutionPyramid {

public:
	/** Constructor.
	 \param width internal processing width
	 \param height internal processing height
	 \param inoutPadding additional padding applied everywhere except on the final result texture
	 \note This is mainly used for the gradient integration task.
	 */
	ConvolutionPyramid(unsigned int width, unsigned int height, unsigned int inoutPadding);

	/** Setup the filters parameters for a given task.
	 \param h1 the 5 coefficients of h1
	 \param h2 the multiplier coefficient h2
	 \param g the 3 coefficients of g
	 \note See Convolution Pyramids, Farbman et al., 2011 for the notation details.
	 */
	void setFilters(const float h1[5], float h2, const float g[3]);

	/** Filter a given input texture.
	 \param textureId the GPU ID of the texture
	 */
	void process(const Texture * textureId);

	/** Cleanup internal resources. */
	void clean() const;

	/** Resize the internal buffers.
	 \param width the new width
	 \param height the new height
	 */
	void resize(unsigned int width, unsigned int height);

	/** The GPU ID of the filter result.
	 \return the ID of the result texture
	 */
	const Texture * textureId() const { return _shifted->textureId(); }

	/** Returns the width expected for the input texture.
	 \return the width expected
	 */
	unsigned int width() { return _resolution[0]; }

	/** Returns the height expected for the input texture.
	 \return the height expected
	 */
	unsigned int height() { return _resolution[1]; }

private:
	const Program * _downscale; ///< Pyramid descending pass shader.
	const Program * _upscale;   ///< Pyramid ascending pass shader.
	const Program * _filter;	///< Filtering shader for the last pyramid level.
	const Program * _padder;	///< Padding helper shader.

	std::unique_ptr<Framebuffer> _shifted;				  ///< Contains the input data padded to the right size.
	std::vector<std::unique_ptr<Framebuffer>> _levelsIn;  ///< The initial levels of the pyramid.
	std::vector<std::unique_ptr<Framebuffer>> _levelsOut; ///< The filtered levels of the pyramid.

	float _h1[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; ///< h1 filter coefficients.
	float _h2	= 0.0f;						   ///< h2 filter multiplier.
	float _g[3]  = {0.0f, 0.0f, 0.0f};			   ///< g filter coefficients.

	glm::ivec2 _resolution = glm::ivec2(0); ///< Resolution expected for the input texture.
	const int _size		   = 5;				///< Size of the filter.
	int _padding		   = 0;				///< Additional padding.
};
