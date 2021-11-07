#pragma once

#include "resources/ResourcesManager.hpp"
#include "resources/Buffer.hpp"
#include "system/Codable.hpp"
#include "Common.hpp"

/**
 \brief Store environment lighting for reflections.
 \ingroup Scene
 */
class LightProbe {

public:
	/** The type of probe. */
	enum class Type : uint {
		STATIC, ///< Loaded from disk, never updated.
		DYNAMIC ///< Generated in engine.
	};

	/** Constructor.
	 */
	explicit LightProbe() = default;

	/** Setup the probe parameters from a list of key-value tuples. The following representations are possible:
	 either:
	 \verbatim
	 * probe:
		radiance: texturetype: ...
		irradiance: shcoeffs_filename
	 \endverbatim
	 for a static environment map or
	 \verbatim
	 * probe:
		position: X,Y,Z
		size: W,H,D
		fade: distance
		center: X,Y,Z
		extent: W,H,D
		rotation: angle
	 \endverbatim
	 for a probe renderered on the fly at the given location.
	 center, extent and rotation are used to define an oriented box used as a local scene proxy. If extent is negative,
	 the environment is assumed to be at infinity.
	 position, size, fade and rotation are used to define a box-shaped area of effect with soft edges.
	 Both size and extent are half-sizes.
	 \param params the parameters tuple
	 \param options data loading and storage options
	 \return decoding status	 
	 */
	bool decode(const KeyValues & params, Storage options);
	
	/** Generate a key-values representation of the probe. See decode for the keywords and layout.
	\return a tuple representing the probe.
	*/
	KeyValues encode() const;

	/** Register an environment, potentially updated on the fly.
	 \param envmap the new map to use
	 \param shCoeffs the new irradiance coefficients
	 */
	void registerEnvironment(const Texture * envmap, const std::shared_ptr<Buffer> & shCoeffs);

	/** Update the area of effect of the probe to ensure it's not bigger than the specified bounding box
	 \param _bbox the maximum size of the probe box
	 */
	void updateSize(const BoundingBox& _bbox);

	/** \return the type of probe
	 */
	Type type() const {
		return _type;
	}

	/** \return the probe position (or the origin for static probes)
	 */
	const glm::vec3 & position() const { return _position; }

	/** \return the half size of the probe area of effect
	 */
	const glm::vec3 & size() const { return _size; }

	/** \return the probe fading band width, at the edges of its area of effect
	 */
	float fade() const { return _fade; }

	/** \return the probe parallax proxy extent half size (or -1 for probes at infinity)
	 */
	const glm::vec3 & extent() const { return _extent; }

	/** \return the probe parallax proxy center
	 */
	const glm::vec3 & center() const { return _center; }

	/** \return the probe orientation.
	 */
	const float & rotation() const { return _rotation; }

	/** \return the probe precomputed orientation (cos(angleY), sin(angleY)).
	 */
	const glm::vec2 & rotationCosSin() const { return _rotCosSin; }

	/** \return the environment map */
	const Texture * map() const { return _envmap; }

	/** \return the irradiance coefficients buffer */
	const std::shared_ptr<Buffer> & shCoeffs() const { return _shCoeffs; }

private:

	const Texture * _envmap = nullptr; ///< The environment map.
	std::shared_ptr<Buffer> _shCoeffs; ///< The irradiance representation.
	
	Type _type = Type::DYNAMIC; ///< The type of probe.
	glm::vec3 _position = glm::vec3(0.0f); ///< The probe location.
	glm::vec3 _size = glm::vec3(1e10f); ///< The probe area of effect.
	glm::vec3 _extent = glm::vec3(-1.0f); ///< The probe parallax proxy extent.
	glm::vec3 _center = glm::vec3(0.0f); ///< The probe parallax proxy center.
	glm::vec2 _rotCosSin = glm::vec2(1.0f, 0.0f); ///< Probe orientation trigonometric cached values.
	float _fade = 1e-8f; ///< The probe effect fading margin around its area of effect.
	float _rotation = 0.0f; ///< The probe orientation around a vertical axis,
};
