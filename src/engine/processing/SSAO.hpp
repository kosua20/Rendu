#pragma once

#include "processing/BoxBlur.hpp"
#include "graphics/Framebuffer.hpp"
#include "Common.hpp"

/**
 \brief Computes screen space ambient occlusion from a depth and view-space normal buffer (brought to [0,1]).
 \see GPU::Frag::SSAO
 \ingroup Processing
 */
class SSAO {

public:
	/**
	 Constructor.
	 \param width the internal resolution width
	 \param height the internal resolution height
	 \param radius the SSAO intersection test radius
	 */
	SSAO(unsigned int width, unsigned int height, float radius);

	/**
	 Compute SSAO using the input depth and normal buffers.
	 \param projection the camera projection matrix
	 \param depthTex the depth texture
	 \param normalTex the view-space normal texture
	 */
	void process(const glm::mat4 & projection, const Texture * depthTex, const Texture * normalTex) const;

	/** Cleanup rssources.
	 */
	void clean() const;

	/**
	 Resize the internal buffers.
	 \param width the new width
	 \param height the new height
	 */
	void resize(unsigned int width, unsigned int height) const;

	/**
	 Clear the final framebuffer texture.
	 */
	void clear() const;

	/**
	 Query the texture containing the result of the SSAO+blur pass.
	 \return the texture
	 */
	const Texture * texture() const;

	/** Query the SSAO radius (should be larger for larger scene with large planar surfaces).
	 \return a reference to the radius parameter
	 */
	float & radius();

private:
	std::unique_ptr<Framebuffer> _ssaoFramebuffer; ///< SSAO framebuffer
	std::unique_ptr<BoxBlur> _blurSSAOBuffer;	  ///< SSAO blur processing.
	Program * _programSSAO;						   ///< The SSAO program.

	float _radius = 0.5f;	///< SSAO intersection test radius.
	Texture _noisetexture; ///< Random noise texture.
};
