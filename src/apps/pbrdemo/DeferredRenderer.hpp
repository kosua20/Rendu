#pragma once

#include "DeferredLight.hpp"
#include "ShadowMap.hpp"

#include "renderers/DebugLightRenderer.hpp"

#include "AmbientQuad.hpp"
#include "scene/Scene.hpp"
#include "renderers/Renderer.hpp"

#include "graphics/Framebuffer.hpp"
#include "input/ControllableCamera.hpp"

#include "processing/GaussianBlur.hpp"
#include "processing/SSAO.hpp"

#include "Common.hpp"

/**
 \brief Available G-buffer layers.
 \ingroup DeferredRendering
 */
enum class TextureType {
	Albedo  = 0, ///< (or base color)
	Normal  = 1,
	Effects = 2, ///< Roughness, metallicness, ambient occlusion factor, ID.
	Depth   = 3
};

/**
 \brief Performs deferred rendering of a scene.
 \ingroup DeferredRendering
 */
class DeferredRenderer final : public Renderer {

public:
	/** Constructor.
	 \param config the configuration to apply when setting up
	 */
	explicit DeferredRenderer(RenderingConfig & config);

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
	
	/** Render the scene to the G-buffer. */
	void renderScene();
	
	/** Render the scene background to the G-buffer. */
	void renderBackground();

	/** Apply the postprocess stack.
	 \param invRenderSize the inverse of the rendering resolution
	 \return the texture containing the result
	 */
	const Texture * renderPostprocess(const glm::vec2 & invRenderSize) const;

	ControllableCamera _userCamera; ///< The interactive camera.

	std::unique_ptr<Framebuffer> _gbuffer;	   ///< G-buffer.
	std::unique_ptr<GaussianBlur> _blurBuffer; ///< Bloom blur processing.
	std::unique_ptr<SSAO> _ssaoPass;		   ///< SSAO processing.

	std::unique_ptr<Framebuffer> _sceneFramebuffer;		  ///< Lighting framebuffer
	std::unique_ptr<Framebuffer> _bloomFramebuffer;		  ///< Bloom framebuffer
	std::unique_ptr<Framebuffer> _toneMappingFramebuffer; ///< Tonemapping framebuffer
	std::unique_ptr<Framebuffer> _fxaaFramebuffer;		  ///< FXAA framebuffer

	std::unique_ptr<AmbientQuad> _ambientScreen; ///< Ambient lighting contribution rendering.
	const Program * _bloomProgram;				 ///< Bloom program
	const Program * _bloomCompositeProgram;		 ///< Bloom compositing program.
	const Program * _toneMappingProgram; ///< Tonemapping program
	const Program * _fxaaProgram;		 ///< FXAA program
	const Program * _finalProgram;		 ///< Final output program
	
	const Program * _objectProgram;		 ///< Basic PBR program
	const Program * _objectNoUVsProgram; ///< Basic PBR program
	const Program * _parallaxProgram;	 ///< Parallax mapping PBR program

	const Program * _skyboxProgram; ///< Skybox program.
	const Program * _bgProgram;		///< Planar background program.
	const Program * _atmoProgram;   ///< Atmospheric scattering program.

	std::shared_ptr<Scene> _scene; 						 ///< The scene to render
	std::unique_ptr<DeferredLight> _lightRenderer;   	 ///< The lights renderer.
	DebugLightRenderer _lightDebugRenderer; 			 ///< The lights debug renderer.
	std::vector<std::unique_ptr<ShadowMap>> _shadowMaps; ///< The lights shadow maps.
	
	glm::vec2 _cplanes = glm::vec2(0.01f, 100.0f); ///< Camera clipping planes.
	float _cameraFOV		 = 50.0f; ///< Camera field of view in degrees.
	float _exposure			 = 1.0f;  ///< Film exposure.
	float _bloomTh			 = 1.2f;  ///< Threshold for blooming regions.
	float _bloomMix			 = 0.2f;  ///< Factor for applying the bloom.
	int _bloomRadius		 = 4;	  ///< Bloom blur radius.
	bool _debugVisualization = false; ///< Toggle the rendering of debug informations in the scene.
	bool _applyBloom		 = true;  ///< Should bloom (bright lights halo-ing) be applied.
	bool _applyTonemapping   = true;  ///< Should HDR to LDR tonemapping be applied.
	bool _applyFXAA			 = true;  ///< Apply screenspace anti-aliasing.
	bool _applySSAO			 = true;  ///< Screen space ambient occlusion.
	bool _updateShadows		 = true;  ///< Update shadow maps at each frame.
	bool _paused			 = false; ///< Pause animations.
};
