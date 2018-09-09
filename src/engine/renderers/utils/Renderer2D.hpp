#ifndef Renderer2D_h
#define Renderer2D_h

#include "../../Common.hpp"
#include "../../Framebuffer.hpp"
#include "../../ScreenQuad.hpp"
#include "../Renderer.hpp"

/**
 \brief Renders a 2D texture with a given shader, for preprocessing.
 \ingroup Renderers
 */
class Renderer2D : public Renderer {

public:

	/** Constructor.
	 \param config the configuration to apply when setting up
	 \param shaderName the name of shader to use
	 \param width the rendering width
	 \param height the rendering height
	 \param preciseFormat the combine type+format to use for the internal famebuffer
	 */
	Renderer2D(Config & config, const std::string & shaderName, const unsigned int width, const unsigned int height, const GLenum preciseFormat);

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
	
	/** Save result to disk.
	 \param outputPath the output image path (no extension)
	 */
	void save(const std::string & outputPath);

	/** Clean internal resources. */
	void clean() const;

	/** Handle a window resize event.
	 \param width the new width
	 \param height the new height
	 */
	void resize(unsigned int width, unsigned int height);
	
	
private:
	
	std::shared_ptr<Framebuffer> _resultFramebuffer; ///< The render framebuffer.
	std::shared_ptr<ProgramInfos> _resultProgram; ///< The rendering program.
	
};

#endif
