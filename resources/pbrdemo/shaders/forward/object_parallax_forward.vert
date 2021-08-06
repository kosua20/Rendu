
// Attributes
layout(location = 0) in vec3 v; ///< Position.
layout(location = 1) in vec3 n; ///< Normal.
layout(location = 2) in vec2 uv; ///< Texture coordinates.
layout(location = 3) in vec3 tang; ///< Tangent.
layout(location = 4) in vec3 binor; ///< Binormal.

layout(set = 0, binding = 1) uniform UniformBlock {
	mat4 mvp; ///< MVP transformation matrix.
	mat4 mv; ///< MV transformation matrix.
	mat4 normalMatrix; ///< Normal transformation matrix.
};

layout(location = 0) out INTERFACE {
    mat4 tbn; ///< Normal to view matrix.
	vec4 tangentSpacePosition; ///< Tangent space position.
	vec4 viewSpacePosition; ///< View space position.
	vec2 uv; ///< UV coordinates.
} Out ;

/** Apply the transformation to the input vertex.
 Compute the tangent-to-view space transformation matrix.
 Output the view space and tangent space positions of the vertex.
 */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);

	Out.uv = uv;

	mat3 nMat = mat3(normalMatrix);
	// Compute the TBN matrix (from tangent space to view space).
	vec3 T = normalize(nMat * tang);
	vec3 B = normalize(nMat * binor);
	vec3 N = normalize(nMat * n);
	mat3 tbn = mat3(T, B, N);
	Out.tbn = mat4(tbn);
	
	Out.viewSpacePosition.xyz = (mv * vec4(v,1.0)).xyz;
	Out.viewSpacePosition.w = 0.0;
	Out.tangentSpacePosition.xyz = transpose(tbn) * Out.viewSpacePosition.xyz;
	Out.tangentSpacePosition.w = 0.0;

	
}
