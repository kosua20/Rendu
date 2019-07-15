#pragma once

#include "scene/Animation.hpp"
#include "Common.hpp"
#include "scene/Object.hpp"
#include "raycaster/Raycaster.hpp"

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
	Light(const glm::vec3& color);
	
	/** Set if the light shoud cast shadows.
	 \param shouldCast toggle shadow casting
	 */
	void castShadow(const bool shouldCast){ _castShadows = shouldCast; }
	
	/** Set the light colored intensity.
	 \param color the new intensity to use
	 */
	void setIntensity(const glm::vec3 & color){ _color = color; }
	
	/** Get the light colored intensity.
	 \return the light intensity
	 */
	const glm::vec3 & intensity(){ return _color; }
	
	/** Add an animation to the light.
	 \param anim the animation to apply
	 */
	void addAnimation(std::shared_ptr<Animation> anim);
	
	/** Perform initialization against the graphics API and register textures for deferred rendering.
	 \param textureIds the IDs of the albedo, normal, depth and effects G-buffer textures
	 */
	virtual void init(const std::vector<GLuint>& textureIds) = 0;
	
	/** Render the light contribution to the scene.
	 \param viewMatrix the current camera view matrix
	 \param projectionMatrix the current camera projection matrix
	 \param invScreenSize the inverse of the textures size
	 */
	virtual void draw( const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize ) const = 0;
	
	/** Render the light shadow map.
	 \param objects list of shadow casting objects to render
	 */
	virtual void drawShadow(const std::vector<Object> & objects) const = 0;
	
	/** Render the light debug wireframe visualisation
	 \param viewMatrix the current camera view matrix
	 \param projectionMatrix the current camera projection matrix
	 */
	virtual void drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const = 0;
	
	/** Apply the animations for a frame duration.
	 \param fullTime the time since the launch of the application
	 \param frameTime the time elapsed since the last frame
	 */
	virtual void update(double fullTime, double frameTime) = 0;
	
	/** Clean internal resources. */
	virtual void clean() const = 0;
	
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
	
	/** Helper that can instantiate a light of any type from the passed keywords and parameters.
	 \param params a key-value tuple containing light parameters
	 \return a generic light pointer
	 */
	static std::shared_ptr<Light> decode(const KeyValues & params);
	
	/** Destructor. */
	virtual ~Light() = default;
	
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
	
	std::vector<GLuint> _textures; ///< The G-buffer textures.
	std::vector<std::shared_ptr<Animation>> _animations; ///< Animations list (will be applied in order).
	
	BoundingBox _sceneBox; ///< The scene bounding box, to fit the shadow map.
	const ProgramInfos * _program; ///< Light rendering program.
	const ProgramInfos * _programDepth; ///< Shadow map program.
	glm::mat4 _mvp; ///< MVP matrix for shadow casting.
	glm::vec3 _color; ///< Colored intensity.
	bool _castShadows; ///< Is the light casting shadows (and thus use a shadow map)..
};
