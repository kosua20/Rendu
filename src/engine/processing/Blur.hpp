#pragma once

#include "graphics/ScreenQuad.hpp"

/**
 \brief A general Blur interface. Can be used to blur a given OpenGL texture and return/draw the result texture.
 \ingroup Processing
 */
class Blur {

public:
	/**
	 Draw the result texture to the current framebuffer.
	 */
	void draw() const;

	/**
	 Query the texture containing the result of the blurring process.
	 \return the texture ID
	 */
	const Texture * textureId() const;

protected:
	/** Constructor.
	 */
	Blur();

	const Texture * _finalTexture;		 ///< The texture holding the blurred result
	const Program * _passthroughProgram; ///< Default passthrough utility program
};
