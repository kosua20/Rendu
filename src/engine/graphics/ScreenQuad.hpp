#pragma once

#include "resources/ResourcesManager.hpp"
#include "Common.hpp"

/**
 \brief Helper used to draw a fullscreen quad for texture processing.
 \details Instead of story two-triangles geometry, it uses an empty vertex array. At renderer time, three invocations of the vertex shader are made.
 The position of each is directly computed from the vertex ID in the shader, so that they generate a triangle covering the screen.
 \see GLSL::Vert::Passthrough
 \see GLSL::Frag::Passthrough, GLSL::Frag::Passthrough_pixelperfect
 \ingroup Graphics
 */
class ScreenQuad {

public:

	/** Draw a full screen quad. */
	static void draw();
	
	/** Draw a full screen quad.
	 \param texture the texture to pass to the shader.
	 */
	static void draw(const Texture & texture);
	
	/** Draw a full screen quad.
	 \param texture the texture to pass to the shader.
	 */
	static void draw(const Texture * texture);
	
	/** Draw a full screen quad.
	 \param textures the textures to pass to the shader.
	 */
	static void draw(const std::vector<const Texture*> & textures);
	
	
private:
	
	/// Constructor.
	ScreenQuad() = default;
	
	static GLuint _vao; ///< The unique empty VAO.
	static bool _init; ///< Has the common VAO been setup?
};
