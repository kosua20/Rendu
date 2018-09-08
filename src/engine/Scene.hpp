#ifndef Scene_h
#define Scene_h
#include "Common.hpp"
#include "Object.hpp"
#include "lights/DirectionalLight.hpp"
#include "lights/PointLight.hpp"
#include "lights/SpotLight.hpp"
#include "resources/ResourcesManager.hpp"
#include <sstream>

/**
 \brief Represents a 2D environment composed of objects, a background and additional environment lighting informations.
 \ingroup Engine
 */
class Scene {

public:

	/** Constructor */
	Scene();
	
	/** Performs initialization against the graphics API.
	 */
	virtual void init() = 0;
	
	/** Update the animations in the scene.
	 \param fullTime the time elapsed since the beginning of the render loop
	 \param frameTime the duration of the last frame
	 */
	virtual void update(double fullTime, double frameTime) = 0;
	
	/** Clean internal resources. */
	void clean() const;
	
	std::vector<Object> objects; ///< The objects in the scene.
	Object background; ///< Background object. \todo Make more flexible, to be able to use a screenquad or a color.
	std::vector<glm::vec3> backgroundIrradiance; ///< RGB SH-coefficients of the background irradiance, computed using SHExtractor. \see SphericalHarmonics
	
	GLuint backgroundReflection; ///< Cubemap texture ID of the background radiance.
	std::vector<DirectionalLight> directionalLights; ///< Directional lights present in the scene.
	std::vector<PointLight> pointLights; ///< Omni-directional lights present in the scene.
	std::vector<SpotLight> spotLights; ///< Spotlights present in the scene.
	
protected:
	
	/** Load a file containing some SH coefficients approximating background irradiance.
	 \param name the name of the text file
	 \see SphericalHarmonics
	 */
	void loadSphericalHarmonics(const std::string & name);
	
	/** Compute the bounding box of the scene, optionaly excluding objects that do not cast shadows.
	 \param onlyShadowCasters denote if only objects that are allowed to cast shadows should be taken into account
	 \return the scene bounding box
	 */
	BoundingBox computeBoundingBox(bool onlyShadowCasters = false);
	
	bool _loaded = false; ///< Has the scene already been loaded from disk.

};
#endif
