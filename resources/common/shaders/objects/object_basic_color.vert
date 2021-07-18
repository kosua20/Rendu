
// Attributes
layout(location = 0) in vec3 v;///< Position.
layout(location = 5) in vec3 c;///< Color.

// Uniform: the MVP.
//uniform mat4 mvp; ///< The transformation matrix.
layout(binding = 0) uniform UniformBlock {
	mat4 mvp;
};

layout(location = 0) out INTERFACE {
	vec3 col; ///< Color.
	vec2 uv;
} Out ;

/** Apply the transformation to the input vertex.
 */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	Out.col = c;
	Out.uv = v.xy * 0.5 + 0.5;
	gl_Position.y *= -1.0;
}
