#pragma once

#include "resources/Texture.hpp"
#include "graphics/Program.hpp"
#include "Common.hpp"

/**
 \brief Apply an approximate bilateral blur to a texture.
 This can be used to blur while preserving edges, or even to upscale and blur a small texture.
 The approximate implementation in Rendu is based on the one demonstrated in G3D (McGuire M., Mara M., Majercik Z., http://casual-effects.com/g3d, 2017), and relies on a separable Gaussian blur with adjusted weights and an extra step distance.
 \ingroup Processing
 */
class BilateralBlur {

public:
	/**
	 Constructor.
	 \param name debug name for internal buffers
	 */
	BilateralBlur(const std::string & name);

	/**
	 Apply the bilateral blur to a texture and write the result in a texture.
	 \note It is possible to use the same texture as input and output.
	 \param projection the camera projection matrix
	 \param src the texture to blur/upscale
	 \param depthTex the depth texture
	 \param normalTex the view-space normal texture
	 \param dst the destination texture
	 */
	void process(const glm::mat4 & projection, const Texture& src, const Texture& depthTex, const Texture& normalTex, Texture & dst);

private:

	/** Resize the internal intermediate buffers.
	 \param width the new resolution width
	 \param height the new resolution height
	 */
	void resize(uint width, uint height);

	Texture _intermediate; ///< Intermediate texture.
	Program * _filter; ///< Bilateral hader.
};
