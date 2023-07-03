#pragma once
#include "system/Config.hpp"
#include "input/Camera.hpp"
#include "resources/Texture.hpp"

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
	 \param dstColor the destination color texture (optional, see outputColorFormat)
	 \param dstDepth the destination depth texture (optional, see outputDepthFormat)
	 \param layer the layer to write to in the target
	 */
	virtual void draw(const Camera & camera, Texture* dstColor, Texture* dstDepth, uint layer = 0);
	
	/** Handle a window resize event.
	 \param width the new width
	 \param height the new height
	 */
	virtual void resize(uint width, uint height);

	/** Display GUI exposing renderer options.
	 \note The renderer can assume that a GUI window is currently open.
	 */
	virtual void interface();

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

	/// \return the preferred format for the destination color rendertarget
	Layout outputColorFormat() const { return _colorFormat; }

	/// \return the preferred format for the destination depth rendertarget
	Layout outputDepthFormat() const { return _depthFormat; }

protected:

	std::string _name; ///< Debug name.
	Layout _colorFormat = Layout::NONE; ///< The preferred output format for a given renderer.
	Layout _depthFormat = Layout::NONE; ///< The preferred output format for a given renderer.
};
