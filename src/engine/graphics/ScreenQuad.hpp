#pragma once

#include "Common.hpp"

/**
 \brief Helper used to draw a fullscreen quad for texture processing.
 \details Instead of story two-triangles geometry, it uses a single triangle covering the whole screen. For instance:
  \verbatim
 2: (-1,3),(0,2)
     *
     | \
     |   \
     |     \
     |       \
     |         \
     *-----------*  1: (3,-1), (2,0)
 0: (-1,-1), (0,0)
 \endverbatim

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
