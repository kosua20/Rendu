
// Attributes
layout(location = 0) in vec3 v; ///< Position.
layout(location = 1) in vec3 n; ///< Normal.
layout(location = 2) in vec2 uv; ///< Texture coordinates.

layout(set = 0, binding = 1) uniform UniformBlock {
	mat4 mvp; ///< MVP transformation matrix.
	mat4 normalMatrix; ///< Normal transformation matrix.
	bool hasUV; ///< Does the mesh have UV.
};

layout(location = 0) out INTERFACE {
	vec4 n; ///< Normal direction.
	vec2 uv; ///< Texture coordinates.
} Out ;

/** Apply the transformation to the input vertex.
 */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);

	Out.uv = hasUV ? uv : vec2(0.5);
	Out.n.xyz = normalize(mat3(normalMatrix) * n);
	Out.n.w = 0.0;
	
}
