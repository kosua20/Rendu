#ifndef RendererCube_h
#define RendererCube_h

#include "../../Common.hpp"
#include "../../graphics/Framebuffer.hpp"
#include "../../Object.hpp"
#include "../Renderer.hpp"

/**
 \brief Renders each face of a cubemap with a given shader, for preprocessing.
 \ingroup Renderers
 */
class RendererCube : public Renderer {

public:

	/** Constructor.
	 \param config the configuration to apply when setting up
	 \param cubemapName the base name of the cubemap to process
	 \param shaderName the name of shader to use
	 \param width the rendering width
	 \param height the rendering height
	 \param preciseFormat the combine type+format to use for the internal famebuffer
	 */
	RendererCube(Config & config, const std::string & cubemapName, const std::string & shaderName, const unsigned int width, const unsigned int height, const GLenum preciseFormat);

	/** Draw the scene and effects */
	void draw();
	
	/** Render, process and save each face of the cubemap.
	 \param localWidth the width of the output image
	 \param localHeight the height of the output image
	 \param localOutputPath the output base image path
	 */
	void drawCube(const unsigned int localWidth, const unsigned int localHeight, const std::string & localOutputPath);
	
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
	void resize(unsigned int width, unsigned  int height);
	
	
private:
	
	std::shared_ptr<Framebuffer> _resultFramebuffer; ///< The internal render framebuffer.
	std::shared_ptr<ProgramInfos> _program; ///< The rendering program to use for each face.
	Object _cubemap; ///< The cubemap object to render for processing.
	
};

#endif
