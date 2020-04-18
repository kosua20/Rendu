#pragma once

#include "scene/lights/Light.hpp"
#include "scene/Animated.hpp"
#include "scene/Object.hpp"
#include "graphics/Framebuffer.hpp"

/**
 \brief A directional light, where all light rays have the same direction.
 \details It can be associated with a shadow 2D map with orthogonal projection. It is rendered as a fullscreen squad in deferred rendering.
 \see GPU::Frag::Directional_light, GPU::Frag::Light_shadow, GPU::Frag::Light_debug
 \ingroup Scene
 */
class DirectionalLight final : public Light {

public:
	/** Default constructor. */
	DirectionalLight() = default;

	/** Constructor.
	 \param worldDirection the light direction in world space
	 \param color the colored intensity of the light
	 */
	DirectionalLight(const glm::vec3 & worldDirection, const glm::vec3 & color);

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
	
	/** Setup a directional light parameters from a list of key-value tuples. The following keywords will be searched for:
	 \verbatim
	 direction: dirX,dirY,dirZ
	 intensity: R,G,B
	 shadows: bool
	 animations:
	 	- animationtype: ...
	 	- ...
	 \endverbatim
	 \param params the parameters tuple
	 */
	void decode(const KeyValues & params);
	
	/**
	 \copydoc Light::encode
	*/
	KeyValues encode() const override;
	
	/** Get the light principal direction in world space.
	 \return the direction
	 */
	const glm::vec3 & direction() const { return _lightDirection; }
	
private:

	glm::mat4 _projectionMatrix = glm::mat4(1.0f);			   ///< Light projection matrix.
	glm::mat4 _viewMatrix		= glm::mat4(1.0f);			   ///< Light view matrix.
	Animated<glm::vec3> _lightDirection { glm::vec3(0.0f, 0.0f, 1.0f) }; ///< Light direction.
};
