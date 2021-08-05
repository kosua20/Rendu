// Attributes
layout(location = 0) in vec3 v;///< Position.
layout(location = 5) in vec3 c;///< Color.

// Uniform: the MVP.
layout(set = 0, binding = 1) uniform UniformBlock {
	mat4 mvp; ///< The transformation matrix.
};

layout(location = 0) out INTERFACE {
	vec3 col; ///< Color.
} Out ;

/** Apply the transformation to the input vertex.
 */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	Out.col = c;
	gl_Position.y *= -1;
}
