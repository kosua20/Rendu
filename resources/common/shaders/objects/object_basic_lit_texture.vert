
// Attributes
layout(location = 0) in vec3 v; ///< Position.
layout(location = 1) in vec3 n; ///< Normal.
layout(location = 2) in vec2 uv; ///< UV.

// Uniform: the MVP.
layout(set = 0, binding = 1) uniform UniformBlock {
	mat4 mvp; ///< The transformation matrix.
	//mat4 normalMatrix; ///< The normal transformation matrix.
};

layout(location = 0) out INTERFACE {
	vec4 vn; ///< World space normal.
	vec2 uv; ///< Texture coordinates.
} Out ;

/** Apply the transformation to the input vertex.
 Compute the world space normal.
 */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	Out.vn.xyz = normalize(n);
	Out.vn.w = 0.0;
	Out.uv = uv;
	
}
