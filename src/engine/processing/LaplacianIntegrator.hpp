#pragma once
#include "ConvolutionPyramid.hpp"
#include "graphics/Framebuffer.hpp"
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
	LaplacianIntegrator(unsigned int width, unsigned int height, unsigned int downscaling);
	
	/** Filter a given input texture, first computing its laplacian field before performing integration.
	 \param textureId the GPU ID of the texture
	 */
	void process(const GLuint textureId);
	
	/** Cleanup internal resources. */
	void clean() const;
	
	/** Resize the internal buffers.
	 \param width the new width
	 \param height the new height
	 */
	void resize(unsigned int width, unsigned int height);
	
	/** The ID of the texture containing the integration result.
	 \return the result texture ID.
	 */
	GLuint textureId(){ return _compo->textureId(); }
	 
	/** The ID of the texture containing the laplacian field.
	\return the laplacian texture ID.
	 */
	GLuint preprocId(){ return _preproc->textureId(); }
	
private:
	
	ConvolutionPyramid _pyramid; ///< The convolution pyramid.
	const ProgramInfos * _prepare; ///< Shader to compute the laplacian field of a RGB image.
	const ProgramInfos * _composite; ///< Passthrough to output the result.
	std::unique_ptr<Framebuffer> _preproc; ///< Contains the computed laplacian field.
	std::unique_ptr<Framebuffer> _compo; ///< Contains the integrated result at input resolution.
	int _scale; ///< The downscaling factor.
	
};

