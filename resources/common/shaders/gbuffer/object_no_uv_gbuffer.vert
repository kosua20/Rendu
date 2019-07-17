#version 330

// Attributes
layout(location = 0) in vec3 v; ///< Position.
layout(location = 1) in vec3 n; ///< Normal.
uniform mat4 mvp; ///< MVP transformation matrix.
uniform mat3 normalMatrix; ///< Normal transformation matrix.

// Output: normal in view space.
out INTERFACE {
    vec3 vn;
} Out ; ///< vec3 vn;

/** Apply the transformation to the input vertex.
  Compute the view space normal.
 */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);

	// Compute the view space normal.
	Out.vn = normalize(normalMatrix * n);
	
}
