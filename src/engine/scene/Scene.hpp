#pragma once

#include "scene/Object.hpp"
#include "scene/lights/DirectionalLight.hpp"
#include "scene/lights/PointLight.hpp"
#include "scene/lights/SpotLight.hpp"
#include "resources/ResourcesManager.hpp"
#include "input/Camera.hpp"
#include "system/Codable.hpp"
#include "Common.hpp"



/**
 \brief Represents a 3D environment composed of objects, a background and additional environment lighting informations.
 \ingroup Scene
 */
class Scene {

public:
	
	/** Constructor from a scene description file. The expected format is as follow:
	 \verbatim
	 * scene:
	 	probe: texturetype: texturename (see Codable::decodeTexture for details)
		irradiance: shcoeffsfilename
		translation: X,Y,Z
		scaling: scale
		orientation: axisX,axisY,axisZ angle
	 * background:
	 	bgtype: value
	 * object:
		... (see Object::decode documentation)
	 ...
	 * lighttype:
		... (see Light::decode documentation)
	 ...
	 \endverbatim
	  where lighttype is one of point, directional, spot and bgtype is one of
	 \verbatim
	 color: R,G,B
	 image: texturetype: texturename
	 cube: texturetype: texturename
	 sun: dirX,dirY,dirZ
	 \endverbatim
	
	 \param name the name of the scene description file
	 */
	Scene(const std::string & name);
	
	/** Performs initialization against the graphics API, loading data.
	 \param mode should the data be stored on the CPU, GPU, or both.
	 */
	void init(const Storage mode);
	
	/** Update the animations in the scene.
	 \param fullTime the time elapsed since the beginning of the render loop
	 \param frameTime the duration of the last frame
	 */
	void update(double fullTime, double frameTime);
	
	/** Clean internal resources. */
	void clean();
	
	/** Get the scene bounding box.
	 \return the bounding box
	 */
	const BoundingBox & boundingBox(){ return _bbox; }
	
	/** Get the initial viewpoint on the scene.
	 \return the viewpoint camera
	 */
	const Camera & viewpoint(){ return _camera; }
	
	std::vector<Object> objects; ///< The objects in the scene.
	std::vector<std::shared_ptr<Light>> lights; ///< Lights present in the scene.
	
	/** \brief The background mode to use for a scene. */
	enum class Background {
		COLOR, ///< Use a unique color as background.
		IMAGE, ///< Use a 2D texture image as background (will be stretched).
		SKYBOX, ///< Use a skybox/cubemap as background.
		ATMOSPHERE ///< Use a realtime atmospheric scattering simulation.
	};
	Background backgroundMode = Background::COLOR; ///< The background mode (see enum).
	glm::vec3 backgroundColor = glm::vec3(0.0f); ///< Color to use if the background mode is COLOR.
	std::unique_ptr<Object> background; ///< Background object, containing the geometry and optional textures to use.
	
	std::vector<glm::vec3> backgroundIrradiance; ///< RGB SH-coefficients of the background irradiance, computed using SHExtractor. \see SphericalHarmonics
	const TextureInfos * backgroundReflection = nullptr; ///< Cubemap texture ID of the background radiance.
	
private:
	
	/** Load an object in the scene from its serialized representation.
	 \param params the object parameters
	 \param mode the storage mode (CPU, GPU, both)
	 */
	void loadObject(const KeyValues & params, const Storage mode);
	
	/** Load a point light in the scene from its serialized representation.
	 \param params the point light parameters
	 \param mode the storage mode (CPU, GPU, both) (unused)
	 */
	void loadLight(const KeyValues & params, const Storage mode);
	
	/** Load the scene camera informations from its serialized representation.
	 \param params the camera parameters
	 \param mode the storage mode (CPU, GPU, both) (unused)
	 */
	void loadCamera(const KeyValues & params, const Storage mode);
	
	/** Load the scene background informations from its serialized representation.
	 \param params the background parameters
	 \param mode the storage mode (CPU, GPU, both)
	 */
	void loadBackground(const KeyValues & params, const Storage mode);
	
	/** Load the scene informations from its serialized representation.
	 \param params the scene parameters
	 \param mode the storage mode (CPU, GPU, both)
	 */
	void loadScene(const KeyValues & params, const Storage mode);
	
	/** Compute the bounding box of the scene, optionaly excluding objects that do not cast shadows.
	 \param onlyShadowCasters denote if only objects that are allowed to cast shadows should be taken into account
	 \return the scene bounding box
	 */
	BoundingBox computeBoundingBox(bool onlyShadowCasters = false);
	
	Camera _camera; ///< The initial viewpoint on the scene.
	BoundingBox _bbox; ///< The scene bounding box.
	glm::mat4 _sceneModel = glm::mat4(1.0f); ///< The scene global transformation.
	std::string _name; ///< The scene file name.
	bool _loaded = false; ///< Has the scene already been loaded from disk.
	
	
};
