#pragma once

#include "scene/lights/Light.hpp"
#include "scene/Object.hpp"
#include "graphics/FramebufferCube.hpp"

#include <array>

/**
 \brief An omnidirectional punctual light, where light is radiating in all directions from a single point in space. Implements distance attenuation.
 \details It can be associated with a shadow cubemap with six orthogonal projections, and is rendered as a sphere in deferred rendering.
 \see GPU::Frag::Point_light, GPU::Frag::Light_shadow_linear, GPU::Frag::Light_debug
 \ingroup Scene
 */
class PointLight final : public Light {

public:
	/** Default constructor. */
	PointLight() = default;

	/** Constructor.
	 \param worldPosition the light position in world space
	 \param color the colored intensity of the light
	 \param radius the distance at which the light is completely attenuated
	 */
	PointLight(const glm::vec3 & worldPosition, const glm::vec3 & color, float radius);

	/**
	 \copydoc Light::draw
	 */
	void draw(const LightRenderer & renderer) const override;

	/**
	 \copydoc Light::update
	 */
	void update(double fullTime, double frameTime) override;

	/**
	 \copydoc Light::setScene
	 */
	void setScene(const BoundingBox & sceneBox) override;

	/**
	 \copydoc Light::visible
	 */
	bool visible(const glm::vec3 & position, const Raycaster & raycaster, glm::vec3 & direction, float & attenuation) const override;

	/** Setup a point light parameters from a list of key-value tuples. The following keywords will be searched for:
	 \verbatim
	 position: X,Y,Z
	 radius: radius
	 intensity: R,G,B
	 shadows: bool
	 animations:
	 	- animationtype: ...
	 	- ...
	 \endverbatim
	 \param params the parameters tuple
	 */
	void decode(const KeyValues & params);

	/** Get the light position in world space.
	 \return the position
	 */
	const glm::vec3 & position() const { return _lightPosition; }
	
	/** Get the light influence radius. No emitted light propagates further than this distance from the light position.
	 \return the radius
	 */
	float radius() const { return _radius; }
	
	/** Get the light far plane used to render the cube shadow map with distances in world space.
	 \return the far plane distance
	 */
	float farPlane() const { return _farPlane; }
	
	/** Get 6 view-projection matrices that cover the full 360° environment, with proper near/far planes for the current environment.
	 \return the matrices list.
	 */
	const std::array<glm::mat4, 6> & vpFaces(){ return _vps; }
	
private:

	std::array<glm::mat4, 6> _vps;				///< Light VP matrices for each face.
	glm::vec3 _lightPosition = glm::vec3(1.0f); ///< Light position.
	float _radius			 = 1.0f;			///< The attenuation radius.
	float _farPlane			 = 1.0f;			///< The projection matrices far plane.

};
