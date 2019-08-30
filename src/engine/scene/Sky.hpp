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
	 \param mode the storage mode (CPU, GPU, both) for the internal data.
	 */
	explicit Sky(Storage mode);
	
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
	 \param mode the storage mode (CPU, GPU, both)
	 */
	void decode(const KeyValues& params, Storage mode) override;
	
	/** Reference to the sun direction.
	 \return the normalized sun direction
	 */
	const glm::vec3 & direction() const { return _sunDirection; }
	
private:
	
	glm::vec3 _sunDirection = glm::vec3(0.0f, 1.0f, 0.0f); ///< The sun direction.
	
};
