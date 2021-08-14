#pragma once

#include "Common.hpp"

/**
 \brief Helper used to draw a fullscreen quad for texture processing.
 \details Instead of story two-triangles geometry, it uses an empty vertex array. At renderer time, three invocations of the vertex shader are made.
 The position of each is directly computed from the vertex ID in the shader, so that they generate a triangle covering the screen.
 \see GPU::Vert::Passthrough
 \see GPU::Frag::Passthrough, GPU::Frag::Passthrough_pixelperfect
 \ingroup Graphics
 */
class ScreenQuad {

public:
	/** Draw a full screen quad. */
	static void draw();

	/// Constructor.
	ScreenQuad() = delete;
	
};
