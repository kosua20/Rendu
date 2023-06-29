#pragma once
#include "processing/ConvolutionPyramid.hpp"
#include "resources/Texture.hpp"
#include "resources/ResourcesManager.hpp"
#include "Common.hpp"

/**
 \brief Compute the laplacian field of a RGB image before reconstructing the initial image through integration, using a filter as described in Convolution Pyramids, Farbman et al., 2011.
 \ingroup Processing
 */
class LaplacianIntegrator {

public:
	/** Constructor.
	 \param width internal processing width
	 \param height internal processing height
	 \param downscaling downscaling factor for the internal convolution pyramid resolution
	 */
	LaplacianIntegrator(uint width, uint height, uint downscaling);

	/** Filter a given input texture, first computing its laplacian field before performing integration.
	 \param texture the GPU ID of the texture
	 */
	void process(const Texture& texture);

	/** Resize the internal buffers.
	 \param width the new width
	 \param height the new height
	 */
	void resize(uint width, uint height);

	/** The ID of the texture containing the integration result.
	 \return the result texture ID.
	 */
	const Texture * texture() const { return &_compo; }

	/** The ID of the texture containing the laplacian field.
	\return the laplacian texture ID.
	 */
	const Texture * preprocId() const { return &_preproc; }

private:
	ConvolutionPyramid _pyramid;		///< The convolution pyramid.
	Program * _prepare;			   		///< Shader to compute the laplacian field of a RGB image.
	Program * _composite;			  	///< Passthrough to output the result.
	Texture _preproc; 					///< Contains the computed laplacian field.
	Texture _compo;   					///< Contains the integrated result at input resolution.
	int _scale;							///< The downscaling factor.
};
