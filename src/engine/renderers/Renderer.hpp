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
	
	/** Default constructor.*/
	Renderer() = default;
	
	/** Draw from a given viewpoint.
	 \param camera the rendering viewpoint
	 \param framebuffer the destination target
	 \param layer the layer to write to in the target
	 */
	virtual void draw(const Camera & camera, Framebuffer & framebuffer, size_t layer = 0);
	
	/** Clean internal resources. */
	virtual void clean();
	
	/** Handle a window resize event.
	 \param width the new width
	 \param height the new height
	 */
	virtual void resize(unsigned int width, unsigned int height);

	/** Display GUI exposing renderer options.
	 \note The renderer can assume that a GUI window is currently open.
	 */
	virtual void interface();

	/** Create a 2D framebuffer with the recommended settings for it to be used as
	 the output of this renderer when calling process/draw.
	 \param width the framebuffer width
	 \param height the framebuffer height
	 \return the allocated framebuffer
	 */
	std::unique_ptr<Framebuffer> createOutput(uint width, uint height) const;

	/** Create a framebuffer with the recommended settings for it to be used as
	the output of this renderer when calling process/draw.
	\param shape the framebuffer texture shape
	\param width the framebuffer width
	\param height the framebuffer height
	\param depth the framebuffer depth
	\param mips the number of mip levels for the framebuffer
	\return the allocated framebuffer
	*/
	std::unique_ptr<Framebuffer> createOutput(TextureShape shape, uint width, uint height, uint depth, uint mips) const;

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

	std::vector<Descriptor> _preferredFormat; ///< The preferred output format for a given renderer.
	bool _needsDepth = false; ///< Does the output needs a depth buffer or not.
};
