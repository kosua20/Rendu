#version 330

// Attributes
layout(location = 0) in vec3 v; ///< Position.
layout(location = 1) in vec3 n; ///< Normal.
layout(location = 2) in vec2 uv; ///< Texture coordinates.
layout(location = 3) in vec3 tang; ///< Tangent.
layout(location = 4) in vec3 binor; ///< Binormal.

uniform mat4 mvp; ///< MVP transformation matrix.
uniform mat3 normalMatrix; ///< Normal transformation matrix.

// Output: tangent space matrix, position in view space and uv.
out INTERFACE {
    mat3 tbn;
	vec2 uv;
} Out ; ///< mat3 tbn; vec2 uv;

/** Apply the transformation to the input vertex.
  Compute the tangent-to-view space transformation matrix.
 */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);

	Out.uv = uv;

	// Compute the TBN matrix (from tangent space to view space).
	vec3 T = normalize(normalMatrix * tang);
	vec3 B = normalize(normalMatrix * binor);
	vec3 N = normalize(normalMatrix * n);
	Out.tbn = mat3(T, B, N);
	
}
