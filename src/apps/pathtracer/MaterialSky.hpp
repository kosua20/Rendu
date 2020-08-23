#pragma once
#include "scene/Scene.hpp"
#include "scene/Sky.hpp"
#include "Common.hpp"

/**
 \brief CPU methods for evaluating the atmospheric scattering model used by the sky background.
 \ingroup PathtracerDemo
 */
class MaterialSky {
public:
	
	/** Constructor. (deleted) */
	MaterialSky() = delete;

	/** Compute the radiance for a given ray, based on the atmosphere scattering model.
		\param rayOrigin the ray origin
		\param rayDir the ray direction
		\param sunDir the light direction
		\return the estimated radiance
	*/
	static glm::vec3 eval(const glm::vec3 & rayOrigin, const glm::vec3 & rayDir, const glm::vec3 & sunDir);

private:

	/** Compute the Rayleigh phase.
		\param cosAngle Cosine of the angle between the ray and the light directions
		\return the phase
	*/
	static float rayleighPhase(float cosAngle);

	/** Compute the Mie phase.
		\param cosAngle Cosine of the angle between the ray and the light directions
		\return the phase
	*/
	static float miePhase(float cosAngle);

	static const Sky::AtmosphereParameters sky; ///< Earth-like atmosphere parameters.
	static const uint samplesCount = 16; ///< Number of samples to evaluate along the ray.

};
