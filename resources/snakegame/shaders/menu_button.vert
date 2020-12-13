// Attributes
layout(location = 0) in vec3 v;///< Position.

// Uniforms.
uniform vec2 position; ///< Image position.
uniform vec2 scale; ///< Image scale.
uniform float depth; ///< Image Z-layer.

/** Compute the position of the button on screen. */
void main(){
	gl_Position.xy = scale * v.xy + position;
	gl_Position.zw = vec2(depth, 1.0);
	
}
