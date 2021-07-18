
// Attributes
layout(location = 0) in vec3 v;///< Position.

// Uniform: the MVP.
//uniform mat4 mvp; ///< The transformation matrix.

layout(binding = 0) uniform UniformBlock {
	mat4 mvp;
};

layout(location = 0) out float vertexID;

/** Apply the MVP transformation to the input vertex. */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	vertexID = float(gl_VertexIndex);
	gl_Position.y *= -1.0;
}
