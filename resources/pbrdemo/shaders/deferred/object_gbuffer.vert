
// Attributes
layout(location = 0) in vec3 v; ///< Position.
layout(location = 1) in vec3 n; ///< Normal.
layout(location = 2) in vec2 uv; ///< Texture coordinates.
layout(location = 3) in vec3 tang; ///< Tangent.
layout(location = 4) in vec3 binor; ///< Binormal.

uniform mat4 mvp; ///< MVP transformation matrix.
uniform mat3 normalMatrix; ///< Normal transformation matrix.
uniform bool hasUV; ///< Does the mesh have texture coordinates.

out INTERFACE {
    mat3 tbn; ///< Normal to view matrix.
	vec2 uv; ///< UV coordinateS.
} Out ;

/** Apply the transformation to the input vertex.
  Compute the tangent-to-view space transformation matrix.
 */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);

	Out.uv = hasUV ? uv : vec2(0.5);

	// Compute the TBN matrix (from tangent space to view space).
	vec3 T = hasUV ? normalize(normalMatrix * tang) : vec3(0.0);
	vec3 B = hasUV ? normalize(normalMatrix * binor) : vec3(0.0);
	vec3 N = normalize(normalMatrix * n);
	Out.tbn = mat3(T, B, N);
	
}
