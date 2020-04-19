#pragma once

#include "graphics/ScreenQuad.hpp"
#include "Common.hpp"

/**
 \brief Renders the ambient lighting contribution of a scene, including irradiance and ambient occlusion.
 \see GPU::Frag::Ambient, GPU::Frag::Ssao
 \ingroup PBRDemo
 */
class AmbientQuad {

public:
	/** Constructor. Setup against the graphics API, register the textures needed.
	 \param texAlbedo the texture containing the albedo
	 \param texNormals the texture containing the surface normals
	 \param texEffects the texture containing the material properties
	 \param texDepth the texture containing the depth
	 \param texSSAO the texture containing the SSAO result
	 */
	AmbientQuad(const Texture * texAlbedo, const Texture * texNormals, const Texture * texEffects, const Texture * texDepth, const Texture * texSSAO);

	/** Register the scene-specific lighting informations.
	 \param irradiance the SH coefficients of the background irradiance
	 */
	void setSceneParameters(const std::vector<glm::vec3> & irradiance);

	/** Draw the ambient lighting contribution to the scene.
	 \param viewMatrix the current camera view matrix
	 \param projectionMatrix the current camera projection matrix
	  \param envmap the environment cubemap (ideally containing radiance convolved with increasing roughness lobes in the mipmap levels)
	 */
	void draw(const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix, const Texture * envmap);

private:
	Program * _program;						///< The ambient lighting program.
	std::vector<const Texture *> _textures; ///< The input textures for the ambient pass.
};
