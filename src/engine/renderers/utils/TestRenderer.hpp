#ifndef TestRenderer_h
#define TestRenderer_h
#include "../../Common.hpp"
#include "../../graphics/Framebuffer.hpp"
#include "../../input/Camera.hpp"
#include "../../graphics/ScreenQuad.hpp"

#include "../../processing/GaussianBlur.hpp"
#include "../../processing/BoxBlur.hpp"

#include "../Renderer.hpp"

#include "../deferred/AmbientQuad.hpp"

/**
 \brief Basic example renderer.
 \ingroup Renderers
 */
class TestRenderer : public Renderer {

public:

	/** Constructor.
	 \param config the configuration to apply when setting up
	 */
	TestRenderer(Config & config);

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
	void resize(unsigned int width, int unsigned height);
	
	
private:
	
	Camera _camera; ///< The user camera.
	std::shared_ptr<Framebuffer> _framebuffer; ///< The render framebuffer.
	std::shared_ptr<ProgramInfos> _program; ///< The rendering program.
	
};

#endif
