#pragma once
#include "PathTracer.hpp"
#include "BVHRenderer.hpp"

#include "Application.hpp"
#include "scene/Scene.hpp"
#include "graphics/Framebuffer.hpp"
#include "input/ControllableCamera.hpp"

#include "Common.hpp"

/**
 \brief Viewer coupled with a basic diffuse path tracer. The user can move the camera anywhere and trigger a path-traced rendering.
 Can also display the raycaster acceleration structure.
 \ingroup PathtracerDemo
 */
class PathTracerApp final : public CameraApp {

public:
	/** Constructor.
	 \param config the configuration to apply
	 \param scene the scene to render
	 */
	explicit PathTracerApp(RenderingConfig & config, const std::shared_ptr<Scene> & scene);

	/** \copydoc CameraApp::draw */
	void draw() override;

	/** \copydoc CameraApp::update
	 */
	void update() override;

	/** \copydoc CameraApp::physics
	 */
	void physics(double fullTime, double frameTime) override;

	/** \copydoc CameraApp::resize
	 */
	void resize() override;

	/** Destructor. */
	~PathTracerApp() override;

private:

	Program * _passthrough;	///< Passthrough program.
	Texture _renderTex;				///< The result texture and image.

	std::shared_ptr<Scene> _scene;				///< The scene to render.
	std::unique_ptr<PathTracer> _pathTracer;	///< The scene specific path tracer.
	std::unique_ptr<BVHRenderer> _bvhRenderer;	///< The scene debug viewer.
	std::unique_ptr<Framebuffer> _sceneFramebuffer; ///< Scene buffer.

	int _samples		 = 8;		///< Samples count.
	int _depth			 = 5;		///< Depth of each ray.
	bool _showRender	 = false;	///< Should the result be displayed.
	bool _lockLevel		 = true;	///< Lock the range of the BVH visualisation.
	bool _liveRender	 = false;	///< Display the result in real-time.
};
