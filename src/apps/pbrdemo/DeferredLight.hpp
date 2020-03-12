#pragma once

#include "scene/Scene.hpp"

#include "renderers/LightRenderer.hpp"
#include "renderers/ShadowMap.hpp"
#include "scene/lights/Light.hpp"
#include "scene/lights/PointLight.hpp"
#include "scene/lights/DirectionalLight.hpp"
#include "scene/lights/SpotLight.hpp"

#include "Common.hpp"


/**
 \brief Apply a light onto the lighting buffer. By processing all lights, the final lighting is accumulated in the buffer.
 \ingroup DeferredRendering
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

	/** Set the current shadowmap settings.
	 \param mode the technique to use
	 \param bias shadow map depth bias
	 */
	void updateShadowMapInfos(ShadowMode mode, float bias);
	
	/** Apply a spot lighting using a supporting cone.
	 \param light the light to compute the contribution of
	 */
	void draw(const SpotLight * light) const override;
	
	/** Apply a point lighting using a supporting sphere.
	 \param light the light to compute the contribution of
	 */
	void draw(const PointLight * light) const override;
	
	/** Apply a directional lighting using a supporting quad.
	 \param light the light to compute the contribution of
	 */
	void draw(const DirectionalLight * light) const override;

private:
	
	std::vector<const Texture *> _textures; ///< G-buffer input textures.
	const Mesh * _sphere; ///< Point light supporting geometry.
	const Mesh * _cone;   ///< Spot light supporting geometry.
	
	const Program * _dirProgram; 	///< Directional light shader.
	const Program * _pointProgram;	///< Point light shader.
	const Program * _spotProgram;	///< Spot light shader.
	
	glm::mat4 _view = glm::mat4(1.0f); ///< Cached camera view matrix.
	glm::mat4 _proj = glm::mat4(1.0f); ///< Cached camera projection matrix.

	ShadowMode _shadowMode = ShadowMode::BASIC; ///< Shadow mapping techique.
	float _shadowBias = 0.0f; ///< Shadow depth bias.
};
