
// Attributes
layout(location = 0) in vec3 v; ///< Position.
layout(location = 2) in vec2 uv; ///< Texture coordinates.

uniform mat4 mvp; ///< MVP transformation matrix.
uniform bool hasUV; ///< Does the mesh ave texture coordinates.

out INTERFACE {
	vec2 uv; ///< UV coordinates.
} Out ;

/** Apply the transformation to the input vertex.
  Compute the tangent-to-view space transformation matrix.
 */
void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);

	Out.uv = hasUV ? uv : vec2(0.5);
	
}
