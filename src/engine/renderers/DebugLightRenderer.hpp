#pragma once

#include "scene/Scene.hpp"

#include "renderers/LightRenderer.hpp"
#include "scene/lights/Light.hpp"
#include "scene/lights/PointLight.hpp"
#include "scene/lights/DirectionalLight.hpp"
#include "scene/lights/SpotLight.hpp"

#include "Common.hpp"

/**
 \brief Visualize lights as colored wireframe objects.
 \ingroup Engine
 */
class DebugLightRenderer final : public LightRenderer {

public:
	/** Constructor. \see GLSL::Frag::light_debug for an example.
	 \param fragmentShader the name of the fragment shader to use.
	 */
	explicit DebugLightRenderer(const std::string & fragmentShader);
	
	/** Set the current user view and projection matrices.
	 \param viewMatrix the camera view matrix
	 \param projMatrix the camera projection matrix
	 */
	void updateCameraInfos(const glm::mat4 & viewMatrix, const glm::mat4 & projMatrix);
	
	/** Draw a spot light as a colored wireframe cone.
	 \param light the light to draw
	 */
	void draw(const SpotLight * light) override;
	
	/** Draw a point light as a colored wireframe sphere.
	 \param light the light to draw
	 */
	void draw(const PointLight * light) override;
	
	/** Draw a directional light as a colored wireframe arrow pointing at the origin.
	 \param light the light to draw
	 */
	void draw(const DirectionalLight * light) override;

private:
	
	const Mesh * _sphere; ///< Point light supporting geometry.
	const Mesh * _cone;   ///< Spot light supporting geometry.
	const Mesh * _arrow;  ///< Spot light supporting geometry.
	
	const Program * _program; ///< Light mesh shader.
	
	glm::mat4 _view = glm::mat4(1.0f); ///< Cached camera view matrix.
	glm::mat4 _proj = glm::mat4(1.0f); ///< Cached camera projection matrix.
};
