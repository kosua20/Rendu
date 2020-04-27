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
	 \endverbatim
	 for a probe renderered on the fly at the given location
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
	void registerEnvironment(const Texture * envmap, const std::shared_ptr<Buffer<glm::vec4>> & shCoeffs);

	/** \return the type of probe
	 */
	Type type() const {
		return _type;
	}

	/** \return the probe position (or the origin for static probes)
	 */
	const glm::vec3 & position() const { return _position; }

	/** \return the environment map */
	const Texture * map() const { return _envmap; }

	/** \return the irradiance coefficients buffer */
	const std::shared_ptr<Buffer<glm::vec4>> & shCoeffs() const { return _shCoeffs; }

private:

	const Texture * _envmap = nullptr; ///< The environment map.
	std::shared_ptr<Buffer<glm::vec4>> _shCoeffs; ///< The irradiance representation.
	
	Type _type = Type::DEFAULT; ///< The type of probe.
	glm::vec3 _position = glm::vec3(0.0f); ///< The probe location.

};
