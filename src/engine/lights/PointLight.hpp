#ifndef PointLight_h
#define PointLight_h
#include "Light.hpp"
#include "../resources/ResourcesManager.hpp"
#include "../FramebufferCube.hpp"
#include "../Object.hpp"

/**
 \brief An omnidirectional punctual light, where light is radiating in all directions from a single point in space. Implements distance attenuation.
 \details It can be associated with a shadow cubemap with six orthogonal projections, and is rendered as a sphere in deferred rendering.
 \see GLSL::Frag::Point_light, GLSL::Frag::Light_shadow_linear, GLSL::Frag::Light_debug
 \ingroup Lights
 */
class PointLight : public Light {

public:
	
	/** Constructor.
	 \param worldPosition the light position in world space
	 \param color the colored intensity of the light
	 \param radius the distance at which the light is completely attenuated
	 \param sceneBox the scene bounding box, for shadow map projection setup
	 */
	PointLight(const glm::vec3& worldPosition, const glm::vec3& color, float radius, const BoundingBox & sceneBox);
	
	/** Perform initialization against the graphics API and register textures for deferred rendering.
	 \param textureIds the IDs of the albedo, normal, depth and effects G-buffer textures
	 */
	void init(const std::vector<GLuint>& textureIds);
	
	/** Render the light contribution to the scene.
	 \param viewMatrix the current camera view matrix
	 \param projectionMatrix the current camera projection matrix
	 \param invScreenSize the inverse of the textures size
	 */
	void draw( const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize ) const;
	
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
	
	/** Update the light position. All internal parameters are updated.
	 \param newPosition the new light position
	 */
	void update(const glm::vec3 & newPosition);
	
	/** Query the current light world space position.
	 \return the current position
	 */
	glm::vec3 position() const { return _lightPosition; }
	
private:
	
	std::shared_ptr<FramebufferCube> _shadowFramebuffer;///< The shadow cubemap framebuffer.
	BoundingBox _sceneBox; ///< The scene bounding box, to fit the shadow map.
	
	std::vector<glm::mat4> _mvps; ///< Light mvp matrices for each face.
	std::vector<glm::mat4> _views; ///< Light view matrices for each face.
	glm::vec3 _lightPosition; ///< Light position.
	float _radius; ///< The attenuation radius.
	float _farPlane; ///< The projection matrices far plane.
	
	MeshInfos _sphere; ///< The supporting geometry.
	std::shared_ptr<ProgramInfos> _program; ///< Light rendering program.
	std::shared_ptr<ProgramInfos> _programDepth; ///< Shadow map program.
	std::vector<GLuint> _textureIds; ///< The G-buffer textures.
	
};

#endif
