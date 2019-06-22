#ifndef PointLight_h
#define PointLight_h
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
class PointLight : public Light {

public:
	
	/** Default constructor. */
	PointLight();
	
	/** Constructor.
	 \param worldPosition the light position in world space
	 \param color the colored intensity of the light
	 \param radius the distance at which the light is completely attenuated
	 */
	PointLight(const glm::vec3& worldPosition, const glm::vec3& color, float radius);
	
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
	
	/** Apply the animations for a frame duration.
	 \param fullTime the time since the launch of the application
	 \param frameTime the time elapsed since the last frame
	 */
	void update(double fullTime, double frameTime);
	
	/** Clean internal resources. */
	void clean() const;
	
	/** Setup a point light parameters from a list of key-value tuples. The following keywords will be searched for:
	 \verbatim
	 position: X,Y,Z
	 radius: radius
	 intensity: R,G,B
	 shadows: bool
	 animations:
	 	animationtype: ...
	 	...
	 \endverbatim
	 \param params the parameters tuples list
	 */
	void decode(const std::vector<KeyValues> & params);
	
	/** Update the scene bounding box used for internal setup (shadow map,...).
	 \param sceneBox the new bounding box
	 */
	void setScene(const BoundingBox & sceneBox);
	
private:
	
	
	std::unique_ptr<FramebufferCube> _shadowFramebuffer;///< The shadow cubemap framebuffer.
	BoundingBox _sceneBox; ///< The scene bounding box, to fit the shadow map.
	
	std::vector<glm::mat4> _mvps; ///< Light mvp matrices for each face.
	glm::vec3 _lightPosition; ///< Light position.
	float _radius; ///< The attenuation radius.
	float _farPlane; ///< The projection matrices far plane.
	
	const MeshInfos * _sphere; ///< The supporting geometry.
	const ProgramInfos * _program; ///< Light rendering program.
	const ProgramInfos * _programDepth; ///< Shadow map program.
	std::vector<GLuint> _textureIds; ///< The G-buffer textures.
	
};

#endif
