
// Attributes
layout(location = 0) in vec3 v;///< Position.
layout(location = 1) in vec3 n; ///< Normal.

layout(set = 0, binding = 1) uniform UniformBlock {
	mat4 mvp; ///< The transformation matrix.
	mat4 normalMat; ///< Model to world space for normals.
};

layout(location = 0) out INTERFACE {
	vec3 n; ///< The world space normal.
} Out;

/** Apply the MVP transformation to the input vertex. */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	Out.n = mat3(normalMat) * n;

	
}
