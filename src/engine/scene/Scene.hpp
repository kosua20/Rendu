#pragma once
#include "scene/Codable.hpp"
#include "scene/Object.hpp"
#include "scene/lights/DirectionalLight.hpp"
#include "scene/lights/PointLight.hpp"
#include "scene/lights/SpotLight.hpp"
#include "resources/ResourcesManager.hpp"
#include "Common.hpp"



/**
 \brief Represents a 3D environment composed of objects, a background and additional environment lighting informations.
 \ingroup Scene
 */
class Scene {

public:

	/** Constructor */
	Scene(const std::string & name);
	
	/** Performs initialization against the graphics API.
	 */
	void init();
	
	/** Update the animations in the scene.
	 \param fullTime the time elapsed since the beginning of the render loop
	 \param frameTime the duration of the last frame
	 */
	void update(double fullTime, double frameTime);
	
	/** Clean internal resources. */
	void clean();
	
	/// Destructor
	virtual ~Scene();
	
	std::vector<Object> objects; ///< The objects in the scene.
	Object background; ///< Background object. \todo Make more flexible, to be able to use a screenquad or a color.
	std::vector<glm::vec3> backgroundIrradiance; ///< RGB SH-coefficients of the background irradiance, computed using SHExtractor. \see SphericalHarmonics
	const TextureInfos * backgroundReflection; ///< Cubemap texture ID of the background radiance.
	std::vector<DirectionalLight> directionalLights; ///< Directional lights present in the scene.
	std::vector<PointLight> pointLights; ///< Omni-directional lights present in the scene.
	std::vector<SpotLight> spotLights; ///< Spotlights present in the scene.
	
protected:
	
	
	std::vector<KeyValues> parse(const std::string & sceneFile);
	
	void loadObject(const std::vector<KeyValues> & params);
	
	void loadPointLight(const std::vector<KeyValues> & params);
	
	void loadDirectionalLight(const std::vector<KeyValues> & params);
	
	void loadSpotLight(const std::vector<KeyValues> & params);
	
	void loadBackground(const std::vector<KeyValues> & params);
		
		
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
	
	std::string _name;
};
