// Attributes
layout(location = 0) in vec3 v;///< Position.

// Uniforms.
layout(set = 0, binding = 1) uniform UniformBlock {
	vec2 position; ///< Image position.
	vec2 scale; ///< Image scale.
	float depth; ///< Image Z-layer.
};

layout(location = 0) out INTERFACE {
	vec2 uv; ///< Texture coordinates.
} Out ;

/** Compute the position of the menu image on screen. */
void main(){
	gl_Position.xy = scale * v.xy + position;
	gl_Position.zw = vec2(depth, 1.0);
	Out.uv = 0.5 * v.xy + 0.5;
	
}
