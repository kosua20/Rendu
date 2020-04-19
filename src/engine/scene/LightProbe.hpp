#pragma once

#include "resources/ResourcesManager.hpp"
#include "system/Codable.hpp"
#include "Common.hpp"

/**
 \brief Store environment lighting for reflections.
 \ingroup Scene
 */
class LightProbe {

public:

	/** Constructor.
	 */
	explicit LightProbe() = default;

	/** Setup the probe parameters from a list of key-value tuples. The following representations are possible:
	 either:
	 \verbatim
	 probe: texturetype: ...
	 \endverbatim
	 for a static environment map or
	 \verbatim
	 probe: position: X,Y,Z
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

	/** Register an environment map, potentially updated on the fly.
	 \param envmap the new map to use
	 */
	void registerEnvmap(const Texture * envmap);

	/** \return true if the map should be generated in-engine
	 */
	bool dynamic() const;

	/** \return the probe position (or the origin for static probes)
	 */
	const glm::vec3 & position() const { return _position; }

	/** \return the environment map */
	const Texture * map() const { return _envmap; }


private:

	/** The type of probe. */
	enum class Type : uint {
		STATIC, ///< Loaded from disk, never updated.
		DYNAMIC ///< Generated in engine.
	};

	const Texture * _envmap = nullptr; ///< The environment map.
	Type _type = Type::STATIC; ///< The type of probe.
	glm::vec3 _position = glm::vec3(0.0f); ///< The probe location.

};
