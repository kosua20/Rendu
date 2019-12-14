#version 330
// Attributes
layout(location = 0) in vec3 v;///< Position.

// Uniforms.
uniform vec2 position; ///< Image position.
uniform vec2 scale; ///< Image scale.
uniform float depth; ///< Image Z-layer.

out INTERFACE {
	vec2 uv; ///< Texture coordinates.
} Out ;

/** Compute the position of the menu image on screen. */
void main(){
	gl_Position.xy = scale * v.xy + position;
	gl_Position.zw = vec2(depth, 1.0);
	Out.uv = 0.5 * v.xy + 0.5;
}
