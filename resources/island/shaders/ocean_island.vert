#version 400

// Attributes
layout(location = 0) in vec3 v; ///< Position.

uniform vec3 shift; ///< Grid shift.

out INTERFACE {
	vec3 pos; ///< World position.
} Out ;

/** Apply the transformation to the input vertex.
  Compute the tangent-to-view space transformation matrix.
 */
void main(){
	// Center to camera location, but only step by integer amount wrt the grid cell size.
	Out.pos = v + vec3(shift.x, 0.0, shift.z);
}
