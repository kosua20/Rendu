#ifndef DeferredRenderer_h
#define DeferredRenderer_h

#include "../../Common.hpp"
#include "../../Framebuffer.hpp"
#include "../../input/ControllableCamera.hpp"
#include "../../ScreenQuad.hpp"

#include "../../processing/GaussianBlur.hpp"
#include "../../processing/BoxBlur.hpp"

#include "../Renderer.hpp"

#include "Gbuffer.hpp"
#include "AmbientQuad.hpp"

/**
 \defgroup DeferredRendering Deferred rendering
 \brief Performs deferred rendering, where geometry and lighting are decoupled by using an intermediate g-buffer.
 \details All scene informations (albedo, normals, material ID, roughness) are rendered to a G-Buffer before being used to render each light contribution using simple geometric proxies.
 \ingroup Renderers
 */

/**
 \brief Performs deferred rendering of a scene.
 \ingroup DeferredRendering
 */
class DeferredRenderer : public Renderer {

public:

	/** Constructor.
	 \param config the configuration to apply when setting up
	 */
	DeferredRenderer(Config & config);

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
	
	ControllableCamera _userCamera; ///< The interactive camera.

	std::shared_ptr<Gbuffer> _gbuffer; ///< G-buffer.
	std::shared_ptr<GaussianBlur> _blurBuffer; ///< Bloom blur processing.
	std::shared_ptr<BoxBlur> _blurSSAOBuffer; ///< SSAO blur processing.
	
	std::shared_ptr<Framebuffer> _ssaoFramebuffer; ///< SSAO framebuffer
	std::shared_ptr<Framebuffer> _sceneFramebuffer; ///< Lighting framebuffer
	std::shared_ptr<Framebuffer> _bloomFramebuffer; ///< Bloom framebuffer
	std::shared_ptr<Framebuffer> _toneMappingFramebuffer; ///< Tonemapping framebuffer
	std::shared_ptr<Framebuffer> _fxaaFramebuffer; ///< FXAA framebuffer
	
	AmbientQuad _ambientScreen; ///< Ambient lighting contribution rendering.
	std::shared_ptr<ProgramInfos> _bloomProgram; ///< Bloom program
	std::shared_ptr<ProgramInfos> _toneMappingProgram; ///< Tonemapping program
	std::shared_ptr<ProgramInfos> _fxaaProgram; ///< FXAA program
	std::shared_ptr<ProgramInfos> _finalProgram; ///< Final output program
	
	std::shared_ptr<Scene> _scene; ///< The scene to render
	
	bool _debugVisualization = false; ///< Toggle the rendering of debug informations.
};

#endif
