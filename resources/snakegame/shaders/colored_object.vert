#version 330

// Attributes
layout(location = 0) in vec3 v;///< Position.
layout(location = 1) in vec3 n; ///< Normal.
layout(location = 2) in vec2 uv; ///< Texture coordinates.

// Uniform: the MVP.
uniform mat4 mvp; ///< The transformation matrix.
uniform mat3 normalMat; ///< Model to world space for normals.

out INTERFACE {
	vec3 n;
} Out;

/** Apply the MVP transformation to the input vertex. */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	Out.n = normalMat*n;
}
