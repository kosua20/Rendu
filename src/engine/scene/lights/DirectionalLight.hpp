#pragma once

#include "scene/lights/Light.hpp"
#include "scene/Object.hpp"
#include "graphics/Framebuffer.hpp"
#include "processing/BoxBlur.hpp"

/**
 \brief A directional light, where all light rays have the same direction.
 \details It can be associated with a shadow 2D map with orthogonal projection, generated using Variance shadow mapping. It is rendered as a fullscreen squad in deferred rendering.
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
	 \copydoc Light::init
	 */
	void init(const Texture * albedo, const Texture * normal, const Texture * depth, const Texture * effects) override;

	/**
	 \copydoc Light::draw
	 */
	void draw(const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix, const glm::vec2 & invScreenSize) const override;

	/**
	 \copydoc Light::drawShadow
	 */
	void drawShadow(const std::vector<Object> & objects) const override;

	/**
	 \copydoc Light::drawDebug
	 */
	void drawDebug(const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix) const override;

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

private:
	std::unique_ptr<Framebuffer> _shadowPass; ///< The shadow map framebuffer.
	std::unique_ptr<BoxBlur> _blur;			  ///< Blur processing for variance shadow mapping.

	glm::mat4 _projectionMatrix = glm::mat4(1.0f);			   ///< Light projection matrix.
	glm::mat4 _viewMatrix		= glm::mat4(1.0f);			   ///< Light view matrix.
	glm::vec3 _lightDirection   = glm::vec3(1.0f, 0.0f, 0.0f); ///< Light direction.
};
