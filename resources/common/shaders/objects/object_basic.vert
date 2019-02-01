#version 330

// Attributes
layout(location = 0) in vec3 v;///< Position.

// Uniform: the MVP.
uniform mat4 mvp; ///< The transformation matrix.

/** Apply the MVP transformation to the input vertex. */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
}
