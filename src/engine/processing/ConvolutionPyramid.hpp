#pragma once
#include "resources/Texture.hpp"
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
	ConvolutionPyramid(uint width, uint height, uint inoutPadding);

	/** Setup the filters parameters for a given task.
	 \param h1 the 5 coefficients of h1
	 \param h2 the multiplier coefficient h2
	 \param g the 3 coefficients of g
	 \note See Convolution Pyramids, Farbman et al., 2011 for the notation details.
	 */
	void setFilters(const float h1[5], float h2, const float g[3]);

	/** Filter a given input texture.
	 \param texture the GPU ID of the texture
	 */
	void process(const Texture& texture);

	/** Resize the internal buffers.
	 \param width the new width
	 \param height the new height
	 */
	void resize(uint width, uint height);

	/** The GPU ID of the filter result.
	 \return the ID of the result texture
	 */
	const Texture * texture() const { return &_shifted; }

	/** Returns the width expected for the input texture.
	 \return the width expected
	 */
	unsigned int width() { return _resolution[0]; }

	/** Returns the height expected for the input texture.
	 \return the height expected
	 */
	unsigned int height() { return _resolution[1]; }

private:
	Program * _downscale; ///< Pyramid descending pass shader.
	Program * _upscale;   ///< Pyramid ascending pass shader.
	Program * _filter;	///< Filtering shader for the last pyramid level.
	Program * _padder;	///< Padding helper shader.

	Texture _shifted;				  ///< Contains the input data padded to the right size.
	std::vector<Texture> _levelsIn;  ///< The initial levels of the pyramid.
	std::vector<Texture> _levelsOut; ///< The filtered levels of the pyramid.

	float _h1[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; ///< h1 filter coefficients.
	float _h2	= 0.0f;						   ///< h2 filter multiplier.
	float _g[3]  = {0.0f, 0.0f, 0.0f};			   ///< g filter coefficients.

	glm::ivec2 _resolution = glm::ivec2(0); ///< Resolution expected for the input texture.
	const int _size		   = 5;				///< Size of the filter.
	int _padding		   = 0;				///< Additional padding.
};
