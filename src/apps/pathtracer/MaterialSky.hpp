#pragma once
#include "scene/Scene.hpp"
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

	/** Check if a sphere of a given radius is intersected by a ray defined by an
		origin wrt to the sphere center and a normalized direction.
		\param rayOrigin the origin of the ray
		\param rayDir the direction of the ray (normalized)
		\param radius the radius of the sphere to intersect
		\param roots will contain the two roots of the associated polynomial, ordered.
		\return true if there is intersection.
		\warning The intersection can be in the negative direction along the ray. Check the sign of the roots to know.
	*/
	static bool intersects(const glm::vec3 & rayOrigin, const glm::vec3 & rayDir, float radius, glm::vec2 & roots);

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

	/** \brief Atmosphere parameters. */
	struct SkyParameters {
		const glm::vec3 sunColor = glm::vec3(1.474f, 1.8504f, 1.91198f); ///< Sun direct color.
		const glm::vec3 kRayleigh = glm::vec3(5.5e-6f, 13.0e-6f, 22.4e-6f); ///< Rayleigh coefficients.
		const float groundRadius = 6371e3f; ///< Radius of the planet.
		const float topRadius = 6471e3f; ///< Radius of the atmosphere.
		const float sunIntensity = 20.0f; ///< Sun intensity.
		const float kMie = 21e-6f; ///< Mie coefficients.
		const float heightRayleigh = 8000.0f; ///< Mie characteristic height.
		const float heightMie = 1200.0f; ///< Mie characteristic height.
		const float gMie = 0.758f; ///< Mie g constant.
		const float sunRadius = 0.04675f; ///< Sun angular radius.
		const float sunRadiusCos = 0.998f; ///< Cosine of the sun angular radius.
		const uint samplesCount = 16; ///< Number of samples to evaluate along the ray.
	};

	static const SkyParameters sky; ///< Earth-like atmosphere parameters.

};
