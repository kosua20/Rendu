#pragma once

#include "PathTracer.hpp"
#include "raycaster/RaycasterVisualisation.hpp"
#include "scene/Scene.hpp"
#include "renderers/Renderer.hpp"
#include "graphics/Framebuffer.hpp"
#include "input/ControllableCamera.hpp"

#include "Common.hpp"

/**
 \brief Renderer coupled with a basic diffuse path tracer. The user can move the camera anywhere and trigger a path-traced rendering.
 Can also display the raycaster acceleration structure.
 \ingroup PathtracerDemo
 */
class BVHRenderer final : public Renderer {

public:
	/** Constructor.
	 \param config the configuration to apply
	 */
	explicit BVHRenderer(RenderingConfig & config);

	/** Set the scene to render.
	 \param scene the new scene
	 */
	void setScene(const std::shared_ptr<Scene> & scene);

	/** Draw the scene and effects */
	void draw() override;

	/** Perform once-per-frame update (buttons, GUI,...) */
	void update() override;

	/** Perform physics simulation update.
	 \param fullTime the time elapsed since the beginning of the render loop
	 \param frameTime the duration of the last frame
	 \note This function can be called multiple times per frame.
	 */
	void physics(double fullTime, double frameTime) override;

	/** Clean internal resources. */
	void clean() override;

	/** Handle a window resize event.
	 \param width the new width
	 \param height the new height
	 */
	void resize(unsigned int width, unsigned int height) override;

private:
	/** Generate visualisation for a ray cast from a given unit viewport position.
	 \param position the position in unit viewport space.
	 */
	void castRay(const glm::vec2 & position);

	ControllableCamera _userCamera; ///< The interactive camera.

	std::unique_ptr<Framebuffer> _sceneFramebuffer; ///< Scene buffer.

	const Program * _objectProgram; ///< Basic object program.
	const Program * _passthrough;   ///< Passthrough program.
	const Program * _bvhProgram;	///< BVH visualisation program.
	std::vector<Mesh> _bvhLevels;   ///< The BVH visualisation mesh.
	std::vector<Mesh> _rayLevels;   ///< BVH nodes intersected with a ray.
	Mesh _rayVis;					///< Mesh representing a ray and its intersected triangle.
	Texture _renderTex;				///< The result texture and image.

	std::shared_ptr<Scene> _scene;						 ///< The scene to render.
	std::unique_ptr<PathTracer> _pathTracer;			 ///< The scene specific path tracer.
	std::unique_ptr<RaycasterVisualisation> _visuHelper; ///< Helper for raycaster internal data visualisation.

	glm::ivec2 _bvhRange = glm::ivec2(0, 1); ///< The subset of the BVH to display.
	float _cameraFOV	 = 70.0f;			 ///< The adjustable camera fov in degrees.
	int _samples		 = 8;				 ///< Samples count.
	int _depth			 = 5;				 ///< Depth of each ray.
	bool _showRender	 = false;			 ///< Should the result be displayed.
	bool _showBVH		 = true;			 ///< Show the raytracer BVH.
	bool _lockLevel		 = true;			 ///< Lock the range of the BVH visualisation.
};
