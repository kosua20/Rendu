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
		DEFAULT, ///< Empty probe.
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
		center: X,Y,Z
		extent: W,H,D
		rotation: angle
	 \endverbatim
	 for a probe renderered on the fly at the given location.
	 center, extent and rotation are used to define an oriented box used as a local scene proxy. If extent is negative,
	 the environment is assumed to be at infinity.
	 \param params the parameters tuple
	 \param options data loading and storage options
	 */
	void decode(const KeyValues & params, Storage options);
	
	/** Generate a key-values representation of the probe. See decode for the keywords and layout.
	\return a tuple representing the probe.
	*/
	KeyValues encode() const;

	/** Register an environment, potentially updated on the fly.
	 \param envmap the new map to use
	 \param shCoeffs the new irradiance coefficients
	 */
	void registerEnvironment(const Texture * envmap, const std::shared_ptr<UniformBuffer<glm::vec4>> & shCoeffs);

	/** \return the type of probe
	 */
	Type type() const {
		return _type;
	}

	/** \return the probe position (or the origin for static probes)
	 */
	const glm::vec3 & position() const { return _position; }

	/** \return the probe parallax proxy extent (or -1 for probes at infinity)
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
	const std::shared_ptr<UniformBuffer<glm::vec4>> & shCoeffs() const { return _shCoeffs; }

private:

	const Texture * _envmap = nullptr; ///< The environment map.
	std::shared_ptr<UniformBuffer<glm::vec4>> _shCoeffs; ///< The irradiance representation.
	
	Type _type = Type::DEFAULT; ///< The type of probe.
	glm::vec3 _position = glm::vec3(0.0f); ///< The probe location.
	glm::vec3 _extent = glm::vec3(-1.0f); ///< The probe parallax proxy extent.
	glm::vec3 _center = glm::vec3(0.0f); ///< The probe parallax proxy center.
	glm::vec2 _rotCosSin = glm::vec2(1.0f, 0.0f); ///< Probe orientation trigonometric cached values.
	float _rotation = 0.0f; ///< The probe orientation around a vertical axis,
};
