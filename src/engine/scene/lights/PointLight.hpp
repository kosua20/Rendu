#pragma once

#include "scene/lights/Light.hpp"
#include "scene/Object.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/FramebufferCube.hpp"

/**
 \brief An omnidirectional punctual light, where light is radiating in all directions from a single point in space. Implements distance attenuation.
 \details It can be associated with a shadow cubemap with six orthogonal projections, and is rendered as a sphere in deferred rendering.
 \see GLSL::Frag::Point_light, GLSL::Frag::Light_shadow_linear, GLSL::Frag::Light_debug
 \ingroup Scene
 */
class PointLight final : public Light {

public:
	
	/** Default constructor. */
	PointLight();
	
	/** Constructor.
	 \param worldPosition the light position in world space
	 \param color the colored intensity of the light
	 \param radius the distance at which the light is completely attenuated
	 */
	PointLight(const glm::vec3& worldPosition, const glm::vec3& color, float radius);
	
	/**
	 \copydoc Light::init
	 */
	void init(const std::vector<const Texture *>& textureIds);
	
	/**
	 \copydoc Light::draw
	 */
	void draw( const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize ) const;
	
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
	
private:
	
	std::unique_ptr<FramebufferCube> _shadowFramebuffer;///< The shadow cubemap framebuffer.
	
	std::vector<glm::mat4> _mvps; ///< Light mvp matrices for each face.
	glm::vec3 _lightPosition; ///< Light position.
	float _radius; ///< The attenuation radius.
	float _farPlane; ///< The projection matrices far plane.
	
	const Mesh * _sphere; ///< The supporting geometry.

};
