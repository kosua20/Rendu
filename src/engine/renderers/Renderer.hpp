#pragma once
#include "system/Config.hpp"
#include "input/Camera.hpp"
#include "resources/Texture.hpp"
#include "graphics/Framebuffer.hpp"

/**
 \brief Base structure of a renderer.
 \ingroup Renderers
 */
class Renderer {
	
public:
	
	/** Default constructor.
	 \param name the debug name of the renderer
	 */
	Renderer(const std::string & name);
	
	/** Draw from a given viewpoint.
	 \param camera the rendering viewpoint
	 \param framebuffer the destination target
	 \param layer the layer to write to in the target
	 */
	virtual void draw(const Camera & camera, Framebuffer & framebuffer, uint layer = 0);
	
	/** Handle a window resize event.
	 \param width the new width
	 \param height the new height
	 */
	virtual void resize(uint width, uint height);

	/** Display GUI exposing renderer options.
	 \note The renderer can assume that a GUI window is currently open.
	 */
	virtual void interface();

	/** Create a 2D framebuffer with the recommended settings for it to be used as
	 the output of this renderer when calling process/draw.
	 \param width the framebuffer width
	 \param height the framebuffer height
	 \param name the framebuffer name
	 \return the allocated framebuffer
	 */
	std::unique_ptr<Framebuffer> createOutput(uint width, uint height, const std::string & name) const;

	/** Create a framebuffer with the recommended settings for it to be used as
	the output of this renderer when calling process/draw.
	\param shape the framebuffer texture shape
	\param width the framebuffer width
	\param height the framebuffer height
	\param depth the framebuffer depth
	\param mips the number of mip levels for the framebuffer
	\param name the framebuffer name
	\return the allocated framebuffer
	*/
	std::unique_ptr<Framebuffer> createOutput(TextureShape shape, uint width, uint height, uint depth, uint mips, const std::string & name) const;

	/** Destructor */
	virtual ~Renderer() = default;
	
	/** Copy constructor.*/
	Renderer(const Renderer &) = delete;
	
	/** Copy assignment.
	 \return a reference to the object assigned to
	 */
	Renderer & operator=(const Renderer &) = delete;
	
	/** Move constructor.*/
	Renderer(Renderer &&) = delete;
	
	/** Move assignment.
	 \return a reference to the object assigned to
	 */
	Renderer & operator=(Renderer &&) = delete;

protected:

	std::string _name; ///< Debug name.
	std::vector<Layout> _preferredFormat; ///< The preferred output format for a given renderer.
};
