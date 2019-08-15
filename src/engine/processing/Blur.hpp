#pragma once

#include "graphics/Framebuffer.hpp"
#include "graphics/ScreenQuad.hpp"
#include "Common.hpp"

/**
 \brief A general Blur interface. Can be used to blur a given OpenGL texture and return/draw the result texture.
 \ingroup Processing
 */
class Blur {

public:

	/**
	 Apply the blurring process to a given texture.
	 \param textureId the ID of the texture to process
	 */
	void process(const Texture * textureId);
	
	/**
	 Draw the result texture to the current framebuffer.
	 */
	void draw();
	
	/** Clean internal resources.
	 */
	void clean() const;

	/** Handle screen resizing if needed.
	 \param width the new width to use
	 \param height the new height to use
	 */
	void resize(unsigned int width, unsigned int height);

	/**
	 Query the texture containing the result of the blurring process.
	 \return the texture ID
	 */
	const Texture * textureId() const;
	
protected:
	
	/** Constructor (disabled)
	 */
	Blur();
	
	const Texture * _finalTexture; ///< The texture holding the blurred result
	const Program * _passthroughProgram; ///< Default passthrough utility program
	
};
