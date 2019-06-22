#ifndef SpotLight_h
#define SpotLight_h

#include "scene/lights/Light.hpp"
#include "scene/Object.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/Framebuffer.hpp"
#include "processing/BoxBlur.hpp"
#include "Common.hpp"

/**
 \brief A spotlight, where light rays in a given cone are radiating from a single point in space. Implements distance attenuation and cone soft transition.
 \details It can be associated with a shadow 2D map with perspective projection, generated using Variance shadow mapping. It is rendered as a cone in deferred rendering.
 \see GLSL::Frag::Spot_light, GLSL::Frag::Light_shadow, GLSL::Frag::Light_debug
 \ingroup Scene
 */
class SpotLight : public Light {

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
	
	/** Perform initialization against the graphics API and register textures for deferred rendering.
	 \param textureIds the IDs of the albedo, normal, depth and effects G-buffer textures
	 */
	void init(const std::vector<GLuint>& textureIds);
	
	/** Render the light contribution to the scene.
	 \param viewMatrix the current camera view matrix
	 \param projectionMatrix the current camera projection matrix
	 \param invScreenSize the inverse of the textures size
	 */
	void draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize) const;
	
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
	
	/** Setup a spot light parameters from a list of key-value tuples. The following keywords will be searched for:
	 \verbatim
	 direction: dirX,dirY,dirZ
	 position: X,Y,Z
	 radius: radius
	 cone: innerAngle outerAngle
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
	
	std::unique_ptr<Framebuffer> _shadowPass; ///< The shadow map framebuffer.
	std::unique_ptr<BoxBlur> _blur; ///< Blur processing for variance shadow mapping.
	BoundingBox _sceneBox; ///< The scene bounding box, to fit the shadow map.
	
	glm::mat4 _projectionMatrix; ///< Light projection matrix.
	glm::mat4 _viewMatrix; ///< Light view matrix.
	glm::vec3 _lightDirection; ///< Light direction.
	glm::vec3 _lightPosition; ///< Light position.
	float _innerHalfAngle; ///< The inner cone attenuation angle.
	float _outerHalfAngle; ///< The outer cone attenuation angle.
	float _radius; ///< The attenuation radius.
	
	const MeshInfos * _cone; ///< The supporting geometry.
	const ProgramInfos * _program; ///< Light rendering program.
	const ProgramInfos * _programDepth; ///< Shadow map program.
	std::vector<GLuint> _textureIds; ///< The G-buffer textures.
	
};

#endif
