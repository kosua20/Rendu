#ifndef DeferredRenderer_h
#define DeferredRenderer_h

#include "AmbientQuad.hpp"
#include "scene/Scene.hpp"
#include "renderers/Renderer.hpp"

#include "graphics/Framebuffer.hpp"
#include "input/ControllableCamera.hpp"
#include "graphics/ScreenQuad.hpp"

#include "processing/GaussianBlur.hpp"
#include "processing/BoxBlur.hpp"
#include "processing/SSAO.hpp"

#include "Common.hpp"

/**
 \brief Available G-buffer layers.
 \ingroup DeferredRendering
 */
enum class TextureType {
	Albedo = 0, ///< (or base color)
	Normal = 1,
	Effects = 2, ///< Roughness, metallicness, ambient occlusion factor, ID.
	Depth = 3
};

/**
 \brief Performs deferred rendering of a scene.
 \ingroup DeferredRendering
 */
class DeferredRenderer : public Renderer {

public:

	/** Constructor.
	 \param config the configuration to apply when setting up
	 */
	DeferredRenderer(RenderingConfig & config);

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
	void clean() const;

	/** Handle a window resize event.
	 \param width the new width
	 \param height the new height
	 */
	void resize(unsigned int width, unsigned int height);
	
	
private:
	
	/** Render the scene to the G-buffer. */
	void renderScene();
	
	/** Apply the postprocess stack.
	 \param invRenderSize the inverse of the rendering resolution
	 \return the ID of the texture containing the result
	 */
	GLuint renderPostprocess(const glm::vec2 & invRenderSize);
	
	ControllableCamera _userCamera; ///< The interactive camera.

	std::unique_ptr<Framebuffer> _gbuffer; ///< G-buffer.
	std::unique_ptr<GaussianBlur> _blurBuffer; ///< Bloom blur processing.
	std::unique_ptr<SSAO> _ssaoPass; ///< SSAO processing.
	
	std::unique_ptr<Framebuffer> _sceneFramebuffer; ///< Lighting framebuffer
	std::unique_ptr<Framebuffer> _bloomFramebuffer; ///< Bloom framebuffer
	std::unique_ptr<Framebuffer> _toneMappingFramebuffer; ///< Tonemapping framebuffer
	std::unique_ptr<Framebuffer> _fxaaFramebuffer; ///< FXAA framebuffer
	
	AmbientQuad _ambientScreen; ///< Ambient lighting contribution rendering.
	const ProgramInfos * _bloomProgram; ///< Bloom program
	const ProgramInfos * _toneMappingProgram; ///< Tonemapping program
	const ProgramInfos * _fxaaProgram; ///< FXAA program
	const ProgramInfos * _finalProgram; ///< Final output program
	const ProgramInfos * _objectProgram; ///< Basic PBR program
	const ProgramInfos * _parallaxProgram;///< Parallax mapping PBR program
	
	const ProgramInfos * _skyboxProgram; ///< Skybox program.
	const ProgramInfos * _bgProgram; ///< Planar background program.
	
	
	std::shared_ptr<Scene> _scene; ///< The scene to render
	
	float _cameraFOV;
	bool _debugVisualization = false; ///< Toggle the rendering of debug informations in the scene.
	bool _applyBloom = true;
	bool _applyTonemapping = true;
	bool _applyFXAA = true;
	bool _applySSAO = true;
	bool _updateShadows = true;
};

#endif
