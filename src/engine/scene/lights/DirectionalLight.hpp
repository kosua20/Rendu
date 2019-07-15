#ifndef DirectionalLight_h
#define DirectionalLight_h

#include "scene/lights/Light.hpp"
#include "scene/Object.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/Framebuffer.hpp"
#include "processing/BoxBlur.hpp"
#include "Common.hpp"

/**
 \brief A directional light, where all light rays have the same direction.
 \details It can be associated with a shadow 2D map with orthogonal projection, generated using Variance shadow mapping. It is rendered as a fullscreen squad in deferred rendering.
 \see GLSL::Frag::Directional_light, GLSL::Frag::Light_shadow, GLSL::Frag::Light_debug
 \ingroup Scene
 */
class DirectionalLight final : public Light {

public:
	
	/** Default constructor. */
	DirectionalLight();
	
	/** Constructor.
	 \param worldDirection the light direction in world space
	 \param color the colored intensity of the light
	 */
	DirectionalLight(const glm::vec3& worldDirection, const glm::vec3& color);
	
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
	std::unique_ptr<BoxBlur> _blur; ///< Blur processing for variance shadow mapping.
	
	glm::mat4 _projectionMatrix; ///< Light projection matrix.
	glm::mat4 _viewMatrix; ///< Light view matrix.
	glm::vec3 _lightDirection; ///< Light direction.
	
};

#endif
