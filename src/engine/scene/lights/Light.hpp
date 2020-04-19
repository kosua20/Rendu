#pragma once

#include "scene/Animation.hpp"
#include "scene/Object.hpp"
#include "raycaster/Raycaster.hpp"
#include "renderers/LightRenderer.hpp"
#include "renderers/shadowmaps/ShadowMap.hpp"
#include "Common.hpp"

enum class LightType : int {
	POINT = 0,
	DIRECTIONAL = 1,
	SPOT = 2
};

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
	virtual void draw(LightRenderer & renderer) = 0;

	/** Apply the animations for a frame duration.
	 \param fullTime the time since the launch of the application
	 \param frameTime the time elapsed since the last frame
	 */
	virtual void update(double fullTime, double frameTime) = 0;

	/** Update the scene bounding box used for internal setup (shadow map,...).
	 \param sceneBox the new bounding box
	 */
	virtual void setScene(const BoundingBox & sceneBox) = 0;

	/** Sample a direction from a reference point to the light.
	 \param position the 3D point
	 \param dist will contain the distance from the light to the point
	 \param attenuation will contain the light attenuation factor
	 \return a direction from the point to the light
	 */
	virtual glm::vec3 sample(const glm::vec3 & position, float & dist, float & attenuation) const = 0;
	
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

	/** Get the light shadow map texture (either 2D or cube depending on the light type) and location.
	 \return the shadow map information
	 */
	const ShadowMap::Region & shadowMap() const { return _shadowMapInfos; }

	/** Set the light shadow map (either 2D or cube depending on the light type).
	 \param map the shadow map texture
	 \param layer the texture layer containing the map
	 \param minUV bottom-left corner of map region in the texture
	 \param maxUV upper-right corner of map region in the texture
	 \warning No check on texture type is performed.
	 */
	void registerShadowMap(const Texture * map, size_t layer = 0, const glm::vec2 & minUV = glm::vec2(0.0f), const glm::vec2 & maxUV = glm::vec2(1.0f)) {
		_shadowMapInfos.map = map;
		_shadowMapInfos.minUV = minUV;
		_shadowMapInfos.maxUV = maxUV;
		_shadowMapInfos.layer = layer;
	}

	/** Check if the light is evolving over time.
	 \return a boolean denoting if animations are applied to the light
	 */
	bool animated() const { return !_animations.empty(); }
	
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
	ShadowMap::Region _shadowMapInfos; ///< Region of the (optional) shadow map containing this light information.
	BoundingBox _sceneBox; 	///< The scene bounding box, to fit the shadow map.
	glm::mat4 _vp;			///< VP matrix for shadow casting.
	glm::mat4 _model;		///< Model matrix of the mesh containing the light-covered region.
	glm::vec3 _color;		///< Colored intensity.
	bool _castShadows;		///< Is the light casting shadows (and thus use a shadow map)..
};
