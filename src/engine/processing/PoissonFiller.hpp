#pragma once
#include "processing/ConvolutionPyramid.hpp"
#include "graphics/Framebuffer.hpp"
#include "resources/ResourcesManager.hpp"
#include "Common.hpp"

/**
\brief Solve a membrane interpolation ("Poisson filling") problem, using a filter as described in Convolution Pyramids, Farbman et al., 2011.
\ingroup Processing
*/
class PoissonFiller {

public:
	
	/** Constructor.
	 \param width internal processing width
	 \param height internal processing height
	 \param downscaling downscaling factor for the internal convolution pyramid resolution
	 */
	PoissonFiller(unsigned int width, unsigned int height, unsigned int downscaling);

	/** Fill black regions of an image in a smooth fashion, first computing its border color before performing filling.
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
	
	/** The ID of the texture containing the filled result.
	 \return the result texture ID.
	 */
	const Texture * textureId() const { return _compo->textureId(); }
	
	/** The ID of the texture containing the colored border.
	 \return the border texture ID.
	 */
	const Texture * preprocId() const { return _preproc->textureId(); }
	
private:
	
	ConvolutionPyramid _pyramid; ///< The convolution pyramid.
	const Program * _prepare; ///< Shader to compute the colored border of black regions in the input image.
	const Program * _composite; ///< Composite the filled field with the input image.
	std::unique_ptr<Framebuffer> _preproc; ///< Contains the computed colored border.
	std::unique_ptr<Framebuffer> _compo;  ///< Contains the composited filled result at input resolution.
	int _scale; ///< The downscaling factor.
	
};

