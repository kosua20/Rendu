#pragma once

#include "scene/Animation.hpp"
#include "scene/Object.hpp"
#include "raycaster/Raycaster.hpp"
#include "renderers/LightRenderer.hpp"
#include "Common.hpp"

/**
 \brief A general light with adjustable color intensity, that can cast shadows.
 \ingroup Scene
 */
class Light {

public:
	/** Default constructor. */
	Light();

	/** Constructor
	 \param color the light color intensity
	 */
	explicit Light(const glm::vec3 & color);
	
	/** Add an animation to the light.
	 \param anim the animation to apply
	 */
	void addAnimation(const std::shared_ptr<Animation> & anim);
	
	/** Process the light using a specific renderer. \see LightRenderer for the expected interface details.
	 \param renderer the light-specific renderer
	 */
	virtual void draw(const LightRenderer & renderer) const = 0;

	/** Apply the animations for a frame duration.
	 \param fullTime the time since the launch of the application
	 \param frameTime the time elapsed since the last frame
	 */
	virtual void update(double fullTime, double frameTime) = 0;

	/** Update the scene bounding box used for internal setup (shadow map,...).
	 \param sceneBox the new bounding box
	 */
	virtual void setScene(const BoundingBox & sceneBox) = 0;

	/** Test the visibility of a point in space from the light source.
	 \param position the 3D point to test
	 \param raycaster the raycaster for intersections tests
	 \param direction will contain the direction from the point to the light
	 \param attenuation will contain the attenuation caused by the radius/cone/etc.
	 \return true if the point is visible from the light source.
	 */
	virtual bool visible(const glm::vec3 & position, const Raycaster & raycaster, glm::vec3 & direction, float & attenuation) const = 0;
	
	/** Generate a key-values representation of the light. See decode for the keywords and layout.
	\return a tuple representing the light
	*/
	virtual KeyValues encode() const;
	
	/** Is the light casting shadows.
	 \return a boolean denoting if the light is a shadowcaster
	 */
	bool castsShadow() const { return _castShadows; }
	
	/** Set if the light should cast shadows.
	 \param shouldCast a boolean denoting if the light is a shadowcaster
	*/
	void setCastShadow(bool shouldCast) { _castShadows = shouldCast; }
	
	/** Get the light colored intensity.
	 \return the light intensity
	 */
	const glm::vec3 & intensity() const { return _color; }
	
	/** Set the light colored intensity.
	\param color the light intensity
	*/
	void setIntensity(const glm::vec3 & color)  { _color = color; }
	
	/** Get the light viewproj matrix.
	 \return the VP matrix
	 */
	const glm::mat4 & vp() const { return _vp; }
	
	/** Get the light mesh model matrix.
	 \return the model matrix
	 */
	const glm::mat4 & model() const { return _model; }
	
	/** Get the light shadow map (either 2D or cube depending on the light type).
	 \return the shadow map texture
	 */
	const Texture * shadowMap() const { return _shadowMap; }
	
	/** Set the light shadow map (either 2D or cube depending on the light type).
	 \param map the shadow map texture
	 \warning No check on texture type is performed.
	 */
	void registerShadowMap(const Texture * map) { _shadowMap = map; }
	
	/** Helper that can instantiate a light of any type from the passed keywords and parameters.
	 \param params a key-value tuple containing light parameters
	 \return a generic light pointer
	 */
	static std::shared_ptr<Light> decode(const KeyValues & params);
	
	/** Destructor. */
	virtual ~Light() = default;

	/** Copy constructor.*/
	Light(const Light &) = delete;

	/** Copy assignment.
	 \return a reference to the object assigned to
	 */
	Light & operator=(const Light &) = delete;

	/** Move constructor.*/
	Light(Light &&) = default;

	/** Move assignment.
	 \return a reference to the object assigned to
	 */
	Light & operator=(Light &&) = delete;

protected:
	/** Setup a light common parameters from a list of key-value tuples. The following keywords will be searched for:
	 \verbatim
	 intensity: R,G,B
	 shadows: bool
	 animations:
	 	- animationtype: ...
	 	- ...
	 ...
	 \endverbatim
	 \param params the parameters tuple list
	 */
	void decodeBase(const KeyValues & params);

	std::vector<std::shared_ptr<Animation>> _animations; ///< Animations list (will be applied in order).
	const Texture * _shadowMap = nullptr;	///< The (optional) light shadow map.
	BoundingBox _sceneBox; 	///< The scene bounding box, to fit the shadow map.
	glm::mat4 _vp;			///< VP matrix for shadow casting.
	glm::mat4 _model;		///< Model matrix of the mesh containing the light-covered region.
	glm::vec3 _color;		///< Colored intensity.
	bool _castShadows;		///< Is the light casting shadows (and thus use a shadow map)..
};
