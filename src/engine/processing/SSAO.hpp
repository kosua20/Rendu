#pragma once

#include "processing/BilateralBlur.hpp"
#include "processing/BoxBlur.hpp"
#include "graphics/Framebuffer.hpp"
#include "resources/Buffer.hpp"
#include "Common.hpp"

/**
 \brief Computes screen space ambient occlusion from a depth and view-space normal buffer (brought to [0,1]).
 \see GPU::Frag::SSAO
 \ingroup Processing
 */
class SSAO {

public:

	/** SSAO upscaling/blurring algorithm. */
	enum class Quality : int {
		LOW = 0, ///< Bilinear upscaling.
		MEDIUM = 1, ///< Approximate box blur and bilinear upscaling.
		HIGH = 2 ///< Bilateral blur.
	};

	/**
	 Constructor.
	 \param width the internal resolution width
	 \param height the internal resolution height
	 \param downscale the downscaling factor for the resolution when computing AO
	 \param radius the SSAO intersection test radius
	 \param name the debug name for internal buffers
	 */
	SSAO(uint width, uint height, uint downscale, float radius, const std::string & name);

	/**
	 Compute SSAO using the input depth and normal buffers.
	 \param projection the camera projection matrix
	 \param depthTex the depth texture
	 \param normalTex the view-space normal texture
	 */
	void process(const glm::mat4 & projection, const Texture * depthTex, const Texture * normalTex);

	/**
	 Resize the internal buffers.
	 \param width the new width
	 \param height the new height
	 */
	void resize(uint width, uint height) const;

	/**
	 Clear the final framebuffer texture.
	 */
	void clear() const;

	/** Destructor. */
	~SSAO();

	/**
	 Query the texture containing the result of the SSAO+blur pass.
	 \return the texture
	 */
	const Texture * texture() const;

	/** Query the SSAO radius (should be larger for larger scene with large planar surfaces).
	 \return a reference to the radius parameter
	 */
	float & radius();

	/** Quality of the blur applied to the SSAO result.
	 \return a reference to the option
	 */
	Quality & quality();

private:
	std::unique_ptr<Framebuffer> _ssaoFramebuffer;  ///< SSAO framebuffer
	std::unique_ptr<Framebuffer> _finalFramebuffer; ///< SSAO framebuffer
	BilateralBlur _highBlur;	  					///< High quality blur.
	BoxBlur _mediumBlur;							///< Medium quality blur.
	Program * _programSSAO;						    ///< The SSAO program.
	UniformBuffer<glm::vec4> _samples; ///< The 3D directional samples.
	Texture _noisetexture = Texture("SSAO noise"); 	///< Random noise texture.
	float _radius = 0.5f;	///< SSAO intersection test radius.
	uint _downscale = 1; 	///< SSAO internal resolution downscaling.
	Quality _quality = Quality::MEDIUM; ///< Quality of the upscaling/blurring.
};
