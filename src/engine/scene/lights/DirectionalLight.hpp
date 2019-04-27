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
 \ingroup Lights
 */
class DirectionalLight : public Light {

public:
	
	/** Constructor.
	 \param worldDirection the light direction in world space
	 \param color the colored intensity of the light
	 \param sceneBox the scene bounding box, for shadow map projection setup
	 */
	DirectionalLight(const glm::vec3& worldDirection, const glm::vec3& color, const BoundingBox & sceneBox);
	
	/** Perform initialization against the graphics API and register textures for deferred rendering.
	 \param textureIds the IDs of the albedo, normal, depth and effects G-buffer textures
	 */
	void init(const std::vector<GLuint>& textureIds);
	
	/** Render the light contribution to the scene.
	 \param viewMatrix the current camera view matrix
	 \param projectionMatrix the current camera projection matrix
	 */
	void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
	
	/** Render the light shadow map.
	 \param objects list of shadow casting objects to render
	 */
	void drawShadow(const std::vector<Object> & objects) const;
	
	/** Render the light debug wireframe visualisation
	 \param viewMatrix the current camera view matrix
	 \param projectionMatrix the current camera projection matrix
	 */
	void drawDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;
	
	/** Clean internal resources. */
	void clean() const;
	
	/** Update the light direction. All internal parameters are updated.
	 \param newDirection the new light direction
	 */
	void update(const glm::vec3 & newDirection);
	
private:
	
	std::unique_ptr<Framebuffer> _shadowPass; ///< The shadow map framebuffer.
	std::unique_ptr<BoxBlur> _blur; ///< Blur processing for variance shadow mapping.
	BoundingBox _sceneBox; ///< The scene bounding box, to fit the shadow map.
	
	glm::mat4 _projectionMatrix; ///< Light projection matrix.
	glm::mat4 _viewMatrix; ///< Light view matrix.
	glm::vec3 _lightDirection; ///< Light direction.
	
	const ProgramInfos * _program; ///< Light rendering program.
	const ProgramInfos * _programDepth; ///< Shadow map program.
	std::vector<GLuint> _textures; ///< The G-buffer textures.
};

#endif
