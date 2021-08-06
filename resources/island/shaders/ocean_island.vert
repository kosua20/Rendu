
// Attributes
layout(location = 0) in vec3 v; ///< Position.

layout(set = 0, binding = 1) uniform UniformBlock {
	vec3 shift; ///< Grid shift.
};

layout(location = 0) out vec3 outPos; ///< World position.

/** Apply the transformation to the input vertex.
  Compute the tangent-to-view space transformation matrix.
 */
void main(){
	// Center to camera location, but only step by integer amount wrt the grid cell size.
	outPos = v + vec3(shift.x, 0.0, shift.z);
}
