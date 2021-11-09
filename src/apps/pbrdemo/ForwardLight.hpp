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


	/** Record spot light information for rendering.
	 \param light the light to compute the contribution of
	 */
	void draw(const SpotLight * light) override;
	
	/** Record point light information for rendering.
	 \param light the light to compute the contribution of
	 */
	void draw(const PointLight * light) override;
	
	/** Record directional light information for rendering.
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

	/** \return the GPU lights recorded buffer */
	UniformBuffer<GPULight> & data(){
		return _lightsData;
	}

private:

	size_t _currentId = 0; ///< Current insertion location.
	size_t _currentCount = 0; ///< Number of lights to store.
	const static size_t _maxLightCount; ///< Maximum allowed number of lights (see forward_lights.glsl).
	UniformBuffer<GPULight> _lightsData; ///< GPU buffer.

	glm::mat4 _view = glm::mat4(1.0f); ///< Cached camera view matrix.
	glm::mat4 _proj = glm::mat4(1.0f); ///< Cached camera projection matrix.
	glm::mat4 _invView = glm::mat4(1.0f); ///< Cached inverse view matrix.

	std::vector<const Texture *> _shadowMaps; ///< Shadow maps list.
};


/**
 \brief Store environment probes data for forward rendering in a GPU buffer.
 \ingroup PBRDemo
 */
class ForwardProbe  {

public:

	/** \brief Represent a probe on the GPU for the forward renderer. */
	struct GPUProbe {
		glm::vec4 positionAndMip{0.0f}; ///< The cubemap location and the mip
		glm::vec4 sizeAndFade{0.0f}; 	///< The cubemap box effect size, and the size of its fading region on edges.
		glm::vec4 centerAndCos{0.0f}; ///< The cubemap parallax box center, and the cubemap parallax box orientation (precomputed cos).
		glm::vec4 extentAndSin{0.0f}; ///< The cubemap parallax box half size, and the cubemap parallax box orientation (precomputed sin).
	};

	/** Constructor.
	 \param count number of probes that will be submitted
	 */
	explicit ForwardProbe(size_t count);

	/** Record a light probe information for rendering.
	 \param probe the probe to retrieve the info of
	 */
	void draw(const LightProbe & probe);

	/** \return the current number of probes */
	size_t count() const {
		return _currentCount;
	}

	/** \return the cubemaps used by the recorded probes */
	const std::vector<const Texture *> & envmaps() const {
		return _probesMaps;
	}

	/** \return the SH irradiance coefficients used by the recorded probes */
	const std::vector<const Buffer *> & shCoeffs() const {
		return _probesCoeffs;
	}

	/** \return the GPU probes recorded buffer */
	UniformBuffer<GPUProbe> & data(){
		return _probesData;
	}

private:

	size_t _currentId = 0; ///< Current insertion location.
	size_t _currentCount = 0; ///< Number of probes to store.
	const static size_t _maxProbeCount; ///< Maximum allowed number of probes (see forward_lights.glsl).
	UniformBuffer<GPUProbe> _probesData; ///< GPU buffer.

	std::vector<const Texture *> _probesMaps; ///< Environment maps list.
	std::vector<const Buffer *> _probesCoeffs; ///< Environment SH coeffs list.
};
