#pragma once

#include "DeferredRenderer.hpp"
#include "ForwardRenderer.hpp"
#include "PostProcessStack.hpp"

#include "Application.hpp"
#include "scene/Scene.hpp"
#include "renderers/Renderer.hpp"
#include "renderers/Probe.hpp"
#include "renderers/shadowmaps/ShadowMap.hpp"
#include "input/ControllableCamera.hpp"

#include "Common.hpp"

/**
 \brief PBR rendering demonstration and interactions.
 \ingroup PBRDemo
 */
class PBRDemo final : public CameraApp {

public:
	/** Constructor.
	 \param config the configuration to apply when setting up
	 */
	explicit PBRDemo(RenderingConfig & config);

	/** \copydoc CameraApp::draw */
	void draw() override;

	/** \copydoc CameraApp::update */
	void update() override;

	/** \copydoc CameraApp::physics */
	void physics(double fullTime, double frameTime) override;

	/** \copydoc CameraApp::clean */
	void clean() override;

	/** \copydoc CameraApp::resize */
	void resize() override;

private:

	/** The type of renderer used to display the scene. */
	enum class RendererMode : int {
		DEFERRED, FORWARD
	};

	/** Select the scene to display.
	 \param scene the scene to use
	 */
	void setScene(const std::shared_ptr<Scene> & scene);

	/** Update the shadow maps and probes. */
	void updateMaps();

	std::vector<std::unique_ptr<ShadowMap>> _shadowMaps; ///< The lights shadow maps.
	std::vector<std::unique_ptr<Probe>> _probes; ///< The environment probes.
	std::unique_ptr<DeferredRenderer> _defRenderer; ///< Deferred PBR renderer.
	std::unique_ptr<ForwardRenderer> _forRenderer;	 ///< Forward PBR renderer.
	std::unique_ptr<PostProcessStack> _postprocess; ///< Post-process renderer.
	std::shared_ptr<DeferredRenderer> _probesRenderer; ///< Renderer for the light probes.
	std::unique_ptr<Framebuffer> _finalRender; ///< Final framebuffer.
	const Program * _finalProgram; ///< Final display program.
	
	std::vector<std::shared_ptr<Scene>> _scenes; ///< The existing scenes.
	std::vector<std::string> _sceneNames; ///< The associated scene names.

	GPUQuery _shadowTime = GPUQuery(GPUQuery::Type::TIME_ELAPSED); ///< Timing for the shadow mapping.
	GPUQuery _probesTime = GPUQuery(GPUQuery::Type::TIME_ELAPSED); ///< Timing for the probe rendering.
	GPUQuery _inteTime = GPUQuery(GPUQuery::Type::TIME_ELAPSED); ///< Timing for the probe preconvolution.
	GPUQuery _copyTime = GPUQuery(GPUQuery::Type::TIME_ELAPSED); ///< Timing for the probe irradiance SH coeffs.
	size_t _copyTimeCPU = 0;  ///< CPU timing for the probe irradiance SH coeffs.
	GPUQuery _rendererTime = GPUQuery(GPUQuery::Type::TIME_ELAPSED); ///< Timing for the scene rendering.
	GPUQuery _postprocessTime = GPUQuery(GPUQuery::Type::TIME_ELAPSED); ///< Timing for the postprocessing.

	RendererMode _mode	 = RendererMode::DEFERRED; ///< Active renderer.
	size_t _currentScene = 0; ///< Currently selected scene.
	glm::vec2 _cplanes  = glm::vec2(0.01f, 100.0f); ///< Camera clipping planes.
	float _cameraFOV	= 50.0f; ///< Camera field of view in degrees.
	const int _frameCount = 2;	 ///< Number of frames to count before looping.
	int _frameID		= 0; 	 ///< Current frame count (will loop)

	bool _paused		= false; ///< Pause animations.
	bool _updateShadows = true; ///< Update the shadow maps.
};
