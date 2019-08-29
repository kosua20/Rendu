#pragma once

#include "scene/lights/Light.hpp"
#include "scene/Object.hpp"
#include "graphics/Framebuffer.hpp"
#include "processing/BoxBlur.hpp"

/**
 \brief A spotlight, where light rays in a given cone are radiating from a single point in space. Implements distance attenuation and cone soft transition.
 \details It can be associated with a shadow 2D map with perspective projection, generated using Variance shadow mapping. It is rendered as a cone in deferred rendering.
 \see GLSL::Frag::Spot_light, GLSL::Frag::Light_shadow, GLSL::Frag::Light_debug
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
	SpotLight(const glm::vec3& worldPosition, const glm::vec3& worldDirection, const glm::vec3& color, float innerAngle, float outerAngle, float radius);
	
	/**
	 \copydoc Light::init
	 */
	void init(const std::vector<const Texture *>& textureIds) override;
	
	/**
	 \copydoc Light::draw
	 */
	void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize) const override;
	
	/**
	 \copydoc Light::drawShadow
	 */
	void drawShadow(const std::vector<Object> & objects) const override;
	
	/**
	 \copydoc Light::drawDebug
	 */
	void drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const override;
	
	/**
	 \copydoc Light::update
	 */
	void update(double fullTime, double frameTime) override;
	
	/**
	 \copydoc Light::clean
	 */
	void clean() const override;
	
	/**
	 \copydoc Light::setScene
	 */
	void setScene(const BoundingBox & sceneBox) override;
	
	/**
	 \copydoc Light::visible
	 */
	bool visible(const glm::vec3 & position, const Raycaster & raycaster, glm::vec3 & direction, float & attenuation) const override;
	
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
	 */
	void decode(const KeyValues & params);
	
private:
	
	std::unique_ptr<Framebuffer> _shadowPass; ///< The shadow map framebuffer.
	std::unique_ptr<BoxBlur> _blur; ///< Blur processing for variance shadow mapping.
	
	glm::mat4 _projectionMatrix = glm::mat4(1.0f); ///< Light projection matrix.
	glm::mat4 _viewMatrix = glm::mat4(1.0f); ///< Light view matrix.
	glm::vec3 _lightDirection = glm::vec3(1.0f, 0.0f, 0.0f); ///< Light direction.
	glm::vec3 _lightPosition = glm::vec3(0.0f); ///< Light position.
	float _innerHalfAngle = float(M_PI) / 4.0f; ///< The inner cone attenuation angle.
	float _outerHalfAngle = float(M_PI) / 2.0f; ///< The outer cone attenuation angle.
	float _radius = 1.0f; ///< The attenuation radius.
	
	const Mesh * _cone = nullptr; ///< The supporting geometry.

};
