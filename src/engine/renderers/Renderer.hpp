#pragma once
#include "system/Config.hpp"
#include "input/Camera.hpp"
#include "resources/Texture.hpp"

/**
 \brief Base structure of a renderer. The result of the renderer is available through result().
 \ingroup Engine
 */
class Renderer {
	
public:
	
	/** Default constructor.*/
	Renderer() = default;
	
	/** Draw from a given viewpoint.
	 \param camera the rendering viewpoint
	 */
	virtual void draw(const Camera & camera);
	
	/** Process a given input texture.
	 \param texture the GPU ID of the texture
	 */
	virtual void process(const Texture * texture);
	
	/** Clean internal resources. */
	virtual void clean() = 0;
	
	/** Handle a window resize event.
	 \param width the new width
	 \param height the new height
	 */
	virtual void resize(unsigned int width, unsigned int height) = 0;

	/** Display GUI exposing renderer options.
	 \note The renderer can assume that a GUI window is currently open.
	 */
	virtual void interface();

	/** Contains the result of the rendering.
	 \return the result texture
	 */
	const Texture * result(){ return _renderResult; }

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
	
	glm::vec2 _renderResolution = glm::vec2(0.0f,0.0f); ///< The internal resolution.
	const Texture * _renderResult = nullptr; ///< The texture containing the result.
};
