
// Attributes
layout(location = 0) in vec3 v;///< Position.
layout(location = 2) in vec2 uv;///< UV.

// Uniform: the MVP.
layout(set = 0, binding = 1) uniform UniformBlock {
	mat4 mvp; ///< The transformation matrix.
};

layout(location = 0) out INTERFACE {
	vec2 uv; ///< Texture coordinates.
} Out;

/** Apply the MVP transformation to the input vertex. */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	Out.uv = uv;
	
}
