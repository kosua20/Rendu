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
	 */
	void decode(const KeyValues & params, Storage options) override;
	
	/** Generate a key-values representation of the object. See decode for the keywords and layout.
	\return a tuple representing the object.
	*/
	KeyValues encode() const override;
	
	/** Reference to the sun direction.
	 \return the normalized sun direction
	 */
	const glm::vec3 & direction() const { return _sunDirection; }

private:
	Animated<glm::vec3> _sunDirection { glm::vec3(0.0f, 1.0f, 0.0f) }; ///< The sun direction.
};
