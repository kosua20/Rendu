#pragma once

#include "scene/lights/Light.hpp"
#include "scene/Animated.hpp"
#include "scene/Object.hpp"

/**
 \brief A spotlight, where light rays in a given cone are radiating from a single point in space. Implements distance attenuation and cone soft transition.
 \details It can be associated with a shadow 2D map with perspective projection, generated using Variance shadow mapping. It is rendered as a cone in deferred rendering.
 \see GPU::Frag::Spot_light, GPU::Frag::Light_shadow, GPU::Frag::Light_debug
 \ingroup Scene
 */
class SpotLight final : public Light {

public:
	/** Default constructor. */
	SpotLight() = default;

	/** Constructor.
	 \param worldPosition the light position in world space
	 \param worldDirection the light cone direction in world space
	 \param color the colored intensity of the light
	 \param innerAngle the inner angle of the cone attenuation
	 \param outerAngle the outer angle of the cone attenuation
	 \param radius the distance at which the light is completely attenuated
	 */
	SpotLight(const glm::vec3 & worldPosition, const glm::vec3 & worldDirection, const glm::vec3 & color, float innerAngle, float outerAngle, float radius);

	/**
	 \copydoc Light::draw
	 */
	void draw(LightRenderer & renderer) override;

	/**
	 \copydoc Light::update
	 */
	void update(double fullTime, double frameTime) override;

	/**
	 \copydoc Light::setScene
	 */
	void setScene(const BoundingBox & sceneBox) override;

	/**
	  \copydoc Light::sample
	 */
	glm::vec3 sample(const glm::vec3 & position, float & dist, float & attenuation) const override;

	/** Setup a spot light parameters from a list of key-value tuples. The following keywords will be searched for:
	 \verbatim
	 direction: dirX,dirY,dirZ
	 position: X,Y,Z
	 radius: radius
	 cone: innerAngle outerAngle
	 intensity: R,G,B
	 shadows: bool
	 animations:
	 	- animationtype: ...
	 	- ...
	 \endverbatim
	 \param params the parameters tuple
	 \return decoding status
	 */
	bool decode(const KeyValues & params);
	
	/**
	 \copydoc Light::encode
	*/
	KeyValues encode() const override;
	
	/** Get the light position in world space.
	 \return the position
	 */
	const glm::vec3 & position() const { return _lightPosition; }
	
	/** Get the light principal direction in world space.
	 \return the direction
	 */
	const glm::vec3 & direction() const { return _lightDirection; }
	
	/** Get the light cone inner and outer angles. Attenuation happens between the two angles.
	 \return the angles
	 */
	const glm::vec2 & angles() const { return _angles; }
	
	/** Get the light influence radius. No emitted light propagates further than this distance from the light position.
	 \return the radius
	 */
	float radius() const { return _radius; }
	
private:

	glm::mat4 _projectionMatrix = glm::mat4(1.0f);			   ///< Light projection matrix.
	glm::mat4 _viewMatrix		= glm::mat4(1.0f);			   ///< Light view matrix.
	Animated<glm::vec3> _lightDirection { glm::vec3(1.0f, 0.0f, 0.0f) }; ///< Light direction.
	Animated<glm::vec3> _lightPosition { glm::vec3(0.0f) };			   	 ///< Light position.
	glm::vec2 _angles 			= glm::vec2(glm::quarter_pi<float>(), glm::half_pi<float>()); ///< The inner and outer cone attenuation angles.
	float _radius				= 1.0f;						   ///< The attenuation radius.
};
