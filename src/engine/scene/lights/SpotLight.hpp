#pragma once

#include "scene/lights/Light.hpp"
#include "scene/Object.hpp"
#include "graphics/ScreenQuad.hpp"
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
	SpotLight();
	
	/** Constructor.
	 \param worldPosition the light position in world space
	 \param worldDirection the light cone direction in world space
	 \param color the colored intensity of the light
	 \param innerAngle the inner angle of the cone attenuation
	 \param outerAngle the outer angle of the cone attenuation
	 \param radius the distance at which the light is completely attenuated
	 */
	SpotLight(const glm::vec3& worldPosition, const glm::vec3& worldDirection, const glm::vec3& color, const float innerAngle, const float outerAngle, const float radius);
	
	/**
	 \copydoc Light::init
	 */
	void init(const std::vector<GLuint>& textureIds);
	
	/**
	 \copydoc Light::draw
	 */
	void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize) const;
	
	/**
	 \copydoc Light::drawShadow
	 */
	void drawShadow(const std::vector<Object> & objects) const;
	
	/**
	 \copydoc Light::drawDebug
	 */
	void drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
	
	/**
	 \copydoc Light::update
	 */
	void update(double fullTime, double frameTime);
	
	/**
	 \copydoc Light::clean
	 */
	void clean() const;
	
	/**
	 \copydoc Light::setScene
	 */
	void setScene(const BoundingBox & sceneBox);
	
	/**
	 \copydoc Light::visible
	 */
	bool visible(const glm::vec3 & position, const Raycaster & raycaster, glm::vec3 & direction, float & attenuation) const;
	
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
	
	glm::mat4 _projectionMatrix; ///< Light projection matrix.
	glm::mat4 _viewMatrix; ///< Light view matrix.
	glm::vec3 _lightDirection; ///< Light direction.
	glm::vec3 _lightPosition; ///< Light position.
	float _innerHalfAngle; ///< The inner cone attenuation angle.
	float _outerHalfAngle; ///< The outer cone attenuation angle.
	float _radius; ///< The attenuation radius.
	
	const MeshInfos * _cone; ///< The supporting geometry.
	
};
