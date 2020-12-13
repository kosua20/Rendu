
// Attributes
layout(location = 0) in vec3 v; ///< Position.
layout(location = 1) in vec3 n; ///< Normal.

uniform mat4 mvp; ///< MVP transformation matrix.
uniform mat4 mv; ///< MV transformation matrix.
uniform mat3 normalMatrix; ///< Normal transformation matrix.

out INTERFACE {
	vec3 viewSpacePosition; ///< View position.
    vec3 vn; ///< Normal in view space.
} Out ;

/** Apply the transformation to the input vertex.
  Compute the view space normal.
 */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	// Store view space position.
	Out.viewSpacePosition = (mv * vec4(v, 1.0)).xyz;
	// Compute the view space normal.
	Out.vn = normalize(normalMatrix * n);
	
}
