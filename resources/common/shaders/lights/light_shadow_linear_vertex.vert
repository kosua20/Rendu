
// Attributes
layout(location = 0) in vec3 v;///< Position.
layout(location = 2) in vec2 uv;///< UV.

// Uniform: the MVP.
uniform mat4 mvp; ///< The transformation matrix.
uniform mat4 m; ///< The model matrix.

out INTERFACE {
	vec3 worldPos; ///< World space position.
	vec2 uv; ///< Texture coordinates.
} Out ;

/** Apply the MVP transformation to the input vertex. */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	Out.worldPos = (m * vec4(v, 1.0)).xyz;
	Out.uv = uv;
}
