#pragma once

#include "scene/Scene.hpp"

#include "renderers/LightRenderer.hpp"
#include "renderers/shadowmaps/ShadowMap.hpp"
#include "scene/lights/Light.hpp"
#include "scene/lights/PointLight.hpp"
#include "scene/lights/DirectionalLight.hpp"
#include "scene/lights/SpotLight.hpp"
#include "resources/Buffer.hpp"

#include "Common.hpp"


/**
 \brief Store lights data for forward rendering in a GPU buffer.
 \ingroup PBRDemo
 */
class ForwardLight final : public LightRenderer {

public:

	/** \brief Represent a light on the GPU for the forward renderer. */
	struct GPULight {
		glm::mat4 viewToLight; ///< View to light matrix.
		glm::vec4 colorAndBias; ///< Light tint and shadow bias.
		glm::vec4 positionAndRadius; ///< Light position and effect radius.
		glm::vec4 directionAndPlane; ///< Light direction and far plane distance.
		glm::vec4 typeModeLayer; ///< Light type, shadow mode and shadow map layer.
		glm::vec4 angles; ///< Cone inner and outer angles.
	};

	/** Constructor.
	 \param count number of lights that will be submitted
	 */
	explicit ForwardLight(size_t count);
	
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

	/** \return the current number of lights */
	size_t count() const {
		return _currentCount;
	}

	/** \return the shadow maps used by the lights */
	const std::vector<const Texture *> & shadowMaps() const {
		return _shadowMaps;
	}

	/** \return the GPU lights buffer */
	Buffer<GPULight> & data(){
		return _lightsData;
	}

private:

	size_t _currentId = 0; ///< Current insertion location.
	size_t _currentCount = 0; ///< Number of lights to store.
	const static size_t _maxLightCount = 50; ///< Maximum allowed number of lights (see common_lights.glsl).
	Buffer<GPULight> _lightsData; ///< GPU buffer.

	glm::mat4 _view = glm::mat4(1.0f); ///< Cached camera view matrix.
	glm::mat4 _proj = glm::mat4(1.0f); ///< Cached camera projection matrix.
	glm::mat4 _invView = glm::mat4(1.0f); ///< Cached inverse view matrix.

	ShadowMode _shadowMode = ShadowMode::BASIC; ///< Shadow mapping techique.
	float _shadowBias = 0.0f; ///< Shadow depth bias.
	std::vector<const Texture *> _shadowMaps; ///< Shadow maps list.
};
