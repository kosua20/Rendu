#version 400

out INTERFACE {
	vec3 pos; ///< Position.
} Out ;

uniform vec3 up; ///< Face vertical vector.
uniform vec3 right; ///< Face horizontal vector.
uniform vec3 center; ///< Face center location.

/**
  Generate one triangle covering the whole screen,
   with according positions and cubemap positions based on vertices ID.
 \sa GLSL::Vert::Passthrough
*/
void main(){
	vec2 temp = 2.0 * vec2(gl_VertexID == 1, gl_VertexID == 2);
	gl_Position.xy = 2.0 * temp - 1.0;
	gl_Position.zw = vec2(1.0);
	Out.pos = center + gl_Position.x * right + gl_Position.y * up;
}
