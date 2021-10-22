
// Attributes
layout(location = 0) in vec3 v; ///< Position.
layout(location = 1) in vec3 n; ///< Normal.
layout(location = 2) in vec2 uv; ///< Texture coordinates.
layout(location = 3) in vec3 tang; ///< Tangent.
layout(location = 4) in vec3 bitan; ///< Bitangent.

layout(set = 0, binding = 1) uniform UniformBlock {
	mat4 mvp; ///< MVP transformation matrix.
	mat4 normalMatrix; ///< Normal transformation matrix.
	bool hasUV; ///< Does the mesh have texture coordinates.
};

layout(location = 0) out INTERFACE {
    mat4 tbn; ///< Normal to view matrix.
	vec4 uv; ///< UV coordinateS.
} Out ;

/** Apply the transformation to the input vertex.
  Compute the tangent-to-view space transformation matrix.
 */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);

	Out.uv.xy = hasUV ? uv : vec2(0.5);
	Out.uv.zw = vec2(0.0);
	
	// Compute the TBN matrix (from tangent space to view space).
	mat3 nMat = mat3(normalMatrix);
	vec3 T = hasUV ? (nMat * tang) : vec3(0.0);
	vec3 B = hasUV ? (nMat * bitan) : vec3(0.0);
	vec3 N = (nMat * n);
	Out.tbn = mat4(mat3(T, B, N));

	
	
}
