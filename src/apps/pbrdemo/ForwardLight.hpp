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
 \brief Store lights data for forward rendering.
 \ingroup DeferredRendering
 */
class ForwardLight final : public LightRenderer {

public:
	/** Constructor.
	 */
	explicit ForwardLight(size_t maxCount);
	
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
	
	
	struct GPULight {
		glm::mat4 viewToLight;
		glm::vec4 colorAndBias;
		glm::vec4 positionAndRadius;
		glm::vec4 directionAndPlane;
		glm::vec2 anglesCos;
		int type;
		int shadowMode;
	};

	std::vector<GPULight> _lightsData;
	size_t _currentId = 0;
	
	glm::mat4 _view = glm::mat4(1.0f); ///< Cached camera view matrix.
	glm::mat4 _proj = glm::mat4(1.0f); ///< Cached camera projection matrix.

	ShadowMode _shadowMode = ShadowMode::BASIC; ///< Shadow mapping techique.
	float _shadowBias = 0.0f; ///< Shadow depth bias.
};
