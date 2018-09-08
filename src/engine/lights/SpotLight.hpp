#ifndef SpotLight_h
#define SpotLight_h

#include "Light.hpp"
#include "../Common.hpp"
#include "../ScreenQuad.hpp"
#include "../Framebuffer.hpp"
#include "../Object.hpp"
#include "../processing/BoxBlur.hpp"

/**
 \brief A spotlight, where light rays in a given cone are radiating from a single point in space. Implements distance attenuation and cone soft transition. It can be associated with a shadow 2D map with perspective projection, generated using Variance shadow mapping. It is rendered as a cone in deferred rendering.
 \ingroup Lights
 */
class SpotLight : public Light {

public:
	
	/** Constructor.
	 \param worldPosition the light position in world space
	 \param worldDirection the light cone direction in world space
	 \param color the colored intensity of the light
	 \param innerAngle the inner angle of the cone attenuation
	 \param outerAngle the outer angle of the cone attenuation
	 \param radius the distance at which the light is completely attenuated
	 \param sceneBox the scene bounding box, for shadow map projection setup
	 */
	SpotLight(const glm::vec3& worldPosition, const glm::vec3& worldDirection, const glm::vec3& color, const float innerAngle, const float outerAngle, const float radius, const BoundingBox & sceneBox);
	
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
	
	/** Clean internal resources. */
	void clean() const;
	
	/** Update the light position. All internal parameters are updated.
	 \param newPosition the new light position
	 */
	void update(const glm::vec3 & newPosition);
	
	/** Update the light position and direction. All internal parameters are updated.
	 \param newPosition the new light position
	 \param newDirection the new light position
	 */
	void update(const glm::vec3 & newPosition, const glm::vec3 & newDirection);
	
	/** Query the current light world space position.
	 \return the current position
	 */
	glm::vec3 position() const { return _lightPosition; }
	
private:
	
	std::shared_ptr<Framebuffer> _shadowPass; ///< The shadow map framebuffer.
	std::shared_ptr<BoxBlur> _blur; ///< Blur processing for variance shadow mapping.
	BoundingBox _sceneBox; ///< The scene bounding box, to fit the shadow map.
	
	glm::mat4 _projectionMatrix; ///< Light projection matrix.
	glm::mat4 _viewMatrix; ///< Light view matrix.
	glm::vec3 _lightDirection; ///< Light direction.
	glm::vec3 _lightPosition; ///< Light position.
	float _innerHalfAngle; ///< The inner cone attenuation angle.
	float _outerHalfAngle; ///< The outer cone attenuation angle.
	float _radius; ///< The attenuation radius.
	
	MeshInfos _cone; ///< The supporting geometry.
	std::shared_ptr<ProgramInfos> _program; ///< Light rendering program.
	std::shared_ptr<ProgramInfos> _programDepth; ///< Shadow map program.
	std::vector<GLuint> _textureIds; ///< The G-buffer textures.
	
};

#endif
