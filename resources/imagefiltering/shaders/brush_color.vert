
// Attributes
layout(location = 0) in vec3 v; ///< Brush vertex position.

layout(set = 0, binding = 1) uniform UniformBlock {
	vec2 radius; ///< Brush radius.
	vec2 position; ///< Brush center position.
};

layout(location = 0) out float dist; ///< Pseudo-distance to the brush center.

/** Place the brush on screen. */
void main(){
	// The distance is 0.0 if the vertex is at the brush center, 1.0 everywhere else.
	dist = all(equal(v.xy, vec2(0.0))) ? 0.0 : 1.0;
	gl_Position.xy = position + radius * v.xy;
	gl_Position.zw = vec2(0.0, 1.0);

	
}

