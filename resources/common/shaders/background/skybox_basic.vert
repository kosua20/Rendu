
// Attributes
layout(location = 0) in vec3 v; ///< Position.

layout(set = 0, binding = 1) uniform UniformBlock {
	mat4 mvp; ///< The transformation matrix.
};

layout(location = 0) out INTERFACE {
	vec3 pos; ///< Position in model space.
} Out;


/** Apply the MVP transformation to the input vertex. */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	Out.pos = v;
}
