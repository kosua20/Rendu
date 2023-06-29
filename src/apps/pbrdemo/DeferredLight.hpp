#pragma once

#include "scene/Scene.hpp"

#include "renderers/LightRenderer.hpp"
#include "renderers/shadowmaps/ShadowMap.hpp"
#include "scene/lights/Light.hpp"
#include "scene/lights/PointLight.hpp"
#include "scene/lights/DirectionalLight.hpp"
#include "scene/lights/SpotLight.hpp"

#include "Common.hpp"


/**
 \brief Apply a light onto the lighting buffer using a geometric proxy (cone, sphere, screenquad). By processing all lights, the final lighting is accumulated in the buffer.
 \sa GPUShaders::Frag::Spot_light_pbr, GPUShaders::Frag::Point_light_pbr, GPUShaders::Frag::Directional_light_pbr
 \ingroup PBRDemo
 */
class DeferredLight final : public LightRenderer {

public:
	/** Constructor.
	 \param texAlbedo the texture containing the albedo
	 \param texNormals the texture containing the surface normals
	 \param texDepth the texture containing the depth
	 \param texEffects the texture containing the material properties
	 */
	explicit DeferredLight(const Texture * texAlbedo, const Texture * texNormals, const Texture * texDepth, const Texture * texEffects);
	
	/** Set the current user view and projection matrices.
	 \param viewMatrix the camera view matrix
	 \param projMatrix the camera projection matrix
	 */
	void updateCameraInfos(const glm::mat4 & viewMatrix, const glm::mat4 & projMatrix);
	
	/** Apply a spot lighting using a supporting cone.
	 \param light the light to compute the contribution of
	 */
	void draw(const SpotLight * light) override;
	
	/** Apply a point lighting using a supporting sphere.
	 \param light the light to compute the contribution of
	 */
	void draw(const PointLight * light) override;
	
	/** Apply a directional lighting using a supporting quad.
	 \param light the light to compute the contribution of
	 */
	void draw(const DirectionalLight * light) override;

private:
	
	std::vector<const Texture *> _textures; ///< G-buffer input textures.
	const Mesh * _sphere; ///< Point light supporting geometry.
	const Mesh * _cone;   ///< Spot light supporting geometry.
	
	Program * _dirProgram; 	///< Directional light shader.
	Program * _pointProgram;	///< Point light shader.
	Program * _spotProgram;	///< Spot light shader.
	
	glm::mat4 _view = glm::mat4(1.0f); ///< Cached camera view matrix.
	glm::mat4 _proj = glm::mat4(1.0f); ///< Cached camera projection matrix.

};


/**
 \brief Apply a probe onto the lighting buffer by rendering a box. The probe contribution weight is accumulated in the alpha channel.
 \sa GPUShaders::Frag::Probe_pbr, GPUShaders::Frag::Probe_normalization
 \ingroup PBRDemo
 */
class DeferredProbe {

public:

	/** Constructor.
	 \param texAlbedo the texture containing the albedo
	 \param texNormals the texture containing the surface normals
	 \param texEffects the texture containing the material properties
	 \param texDepth the texture containing the depth	 
	 \param texSSAO the texture containing the SSAO result
	 */
	explicit DeferredProbe(const Texture * texAlbedo, const Texture * texNormals, const Texture * texEffects, const Texture * texDepth, const Texture * texSSAO);

	/** Set the current user view and projection matrices.
	 \param viewMatrix the camera view matrix
	 \param projMatrix the camera projection matrix
	 */
	void updateCameraInfos(const glm::mat4 & viewMatrix, const glm::mat4 & projMatrix);

	/** Apply a probe in the current render destination
	 \param probe the probe to compute the contribution of
	 */
	void draw(const LightProbe & probe);

private:

	std::vector<const Texture *> _textures; ///< G-buffer input textures.
	const Mesh * _box; ///< Probe supporting geometry.
	Program * _program; ///< Probe application shader.

	glm::mat4 _viewProj = glm::mat4(1.0f); ///< Cached camera view projection matrix.
	glm::mat4 _invView = glm::mat4(1.0f); ///< Cached camera inverse view matrix.
	glm::vec4 _projectionVector = glm::vec4(0.0f); ///< Cached camera projection parameters.
};
