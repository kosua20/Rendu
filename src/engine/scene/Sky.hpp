#pragma once

#include "scene/Object.hpp"
#include "resources/ResourcesManager.hpp"
#include "system/Codable.hpp"
#include "Common.hpp"

/**
 \brief Represent a background environment with atmospheric scattering.
 The sun direction can be animated.
 \ingroup Scene
 \see AtmosphericScattering
 */
class Sky final : public Object {

public:
	/** Constructor.
	 \param options data loading and storage options
	 */
	explicit Sky(Storage options);

	/** Apply the animations for a frame duration.
	 \param fullTime the time since the launch of the application
	 \param frameTime the time elapsed since the last frame
	 */
	void update(double fullTime, double frameTime) override;

	/** Setup a sky environment parameters from a list of key-value tuples. The following keywords will be searched for:
	 \verbatim
	 bgsky:
	 	direction: X,Y,Z
	 	animations:
	 		...
	 \endverbatim
	 \param params the parameters tuple
	 \param options data loading and storage options
	 \return decoding status
	 */
	bool decode(const KeyValues & params, Storage options) override;
	
	/** Generate a key-values representation of the object. See decode for the keywords and layout.
	\return a tuple representing the object.
	*/
	KeyValues encode() const override;
	
	/** Reference to the sun direction.
	 \return the normalized sun direction
	 */
	const glm::vec3 & direction() const { return _sunDirection; }

	/** \brief Atmosphere parameters. Default values correspond to Earth-like atmosphere.
	 */
	struct AtmosphereParameters {
		glm::vec3 sunColor = glm::vec3(1.474f, 1.8504f, 1.91198f); ///< Sun direct color.
		glm::vec3 kRayleigh = glm::vec3(5.5e-6f, 13.0e-6f, 22.4e-6f); ///< Rayleigh coefficients.
		float groundRadius = 6371e3f; ///< Radius of the planet.
		float topRadius = 6471e3f; ///< Radius of the atmosphere.
		float sunIntensity = 20.0f; ///< Sun intensity.
		float kMie = 21e-6f; ///< Mie coefficients.
		float heightRayleigh = 8000.0f; ///< Mie characteristic height.
		float heightMie = 1200.0f; ///< Mie characteristic height.
		float gMie = 0.758f; ///< Mie g constant.
		float sunRadius = 0.04675f; ///< Sun angular radius.
		float sunRadiusCos = 0.998f; ///< Cosine of the sun angular radius.
	};

private:
	Animated<glm::vec3> _sunDirection { glm::vec3(0.0f, 1.0f, 0.0f) }; ///< The sun direction.
};
