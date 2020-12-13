
layout(location = 0) in vec3 v; ///< Position.

uniform mat4 mvp; ///< Transformation matrix.
uniform vec3 camPos; ///< Camera world position.

out INTERFACE {
	vec3 pos; ///< World position.
	vec3 srcPos; ///< World position before waves perturbation.
	vec3 prevPos; ///< World position before small scale waves perturbation.
} Out;

/** Project the cylinder geoemtry onto the far plane of the scene, centered on the camera. */
void main(){
	vec3 worldPos = v + vec3(camPos.x, 0.0, camPos.z);
	gl_Position = mvp * vec4(worldPos, 1.0);
	// Send to almost maximal depth.
	gl_Position.z = gl_Position.w*0.9999;
	Out.pos = worldPos;
	Out.srcPos = worldPos;
	Out.prevPos = worldPos;
}
