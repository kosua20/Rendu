#version 400

// Attributes
layout(location = 0) in vec3 v; ///< Position.

uniform mat4 mvp; ///< MVP transformation matrix.
uniform vec3 shift;
uniform vec3 camPos;

out INTERFACE {
	vec3 pos;
} Out ;

/** Apply the transformation to the input vertex.
  Compute the tangent-to-view space transformation matrix.
 */
void main(){
	// Scale based on the camera distance above the ocean plane.
	float scale = exp2(max(floor(log2(abs(camPos.y)))-3.0, 0.0));
	// Center to camera location, but only step by integer amount wrt the grid cell size.
	vec2 rshift = round(shift.xz/scale);
	Out.pos = scale * (v + vec3(rshift.x, 0.0, rshift.y));
}
