#pragma once

#include "PathTracer.hpp"
#include "raycaster/RaycasterVisualisation.hpp"
#include "scene/Scene.hpp"
#include "renderers/Renderer.hpp"
#include "graphics/Framebuffer.hpp"
#include "input/ControllableCamera.hpp"
#include "graphics/ScreenQuad.hpp"

#include "Common.hpp"


/**
 \brief Renderer coupled with a basic diffuse path tracer. The user can move the camera anywhere and trigger a path-traced rendering.
 Can also display the raycaster acceleration structure.
 \ingroup PathtracerDemo
 */
class BVHRenderer : public Renderer {

public:

	/** Constructor.
	 \param config the configuration to apply when setting up
	 */
	BVHRenderer(RenderingConfig & config);

	/** Set the scene to render.
	 \param scene the new scene
	 */
	void setScene(std::shared_ptr<Scene> scene);
	
	/** Draw the scene and effects */
	void draw();
	
	/** Perform once-per-frame update (buttons, GUI,...) */
	void update();
	
	/** Perform physics simulation update.
	 \param fullTime the time elapsed since the beginning of the render loop
	 \param frameTime the duration of the last frame
	 \note This function can be called multiple times per frame.
	 */
	void physics(double fullTime, double frameTime);

	/** Clean internal resources. */
	void clean();

	/** Handle a window resize event.
	 \param width the new width
	 \param height the new height
	 */
	void resize(unsigned int width, unsigned int height);
	
	
private:
	
	/** Generate visualisation for a ray cast from a given unit viewport position. */
	void castRay(const glm::vec2 & position);
	
	ControllableCamera _userCamera; ///< The interactive camera.

	std::unique_ptr<Framebuffer> _sceneFramebuffer; ///< Scene buffer.
	
	const ProgramInfos * _objectProgram; ///< Basic object program.
	const ProgramInfos * _passthrough; ///< Passthrough program.
	const ProgramInfos * _bvhProgram; ///< BVH visualisation program.
	std::vector<Mesh> _bvhLevels; ///< The BVH visualisation mesh.
	std::vector<Mesh> _rayLevels; ///< BVH nodes intersected with a ray.
	Mesh _rayVis; ///< Mesh representing a ray and its intersected triangle.
	Texture _renderTex; ///< The result texture and image.
	
	std::shared_ptr<Scene> _scene; ///< The scene to render.
	PathTracer _pathTracer; ///< The scene specific path tracer.
	std::unique_ptr<RaycasterVisualisation> _visuHelper; ///< Helper for raycaster internal data visualisation.
	
	glm::ivec2 _bvhRange; ///< The subset of the BVH to display.
	float _cameraFOV; ///< The adjustable camera fov in degrees.
	int _samples = 8; ///< Samples count.
	int _depth = 5; ///< Depth of each ray.
	bool _showRender = false; ///< Should the result be displayed.
	bool _showBVH = true; ///< Show the raytracer BVH.
	bool _lockLevel = true; ///< Lock the range of the BVH visualisation.
};
