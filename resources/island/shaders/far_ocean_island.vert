
layout(location = 0) in vec3 v; ///< Position.

layout(set = 0, binding = 1) uniform UniformBlock {
	mat4 mvp; ///< Transformation matrix.
	vec3 camPos; ///< Camera world position.
};


layout(location = 0) out vec3 oPos; ///< World position.
layout(location = 1) out vec3 oSrcPos; ///< World position before waves perturbation.
layout(location = 2) out vec3 oPrevPos; ///< World position before small scale waves perturbation.


/** Project the cylinder geoemtry onto the far plane of the scene, centered on the camera. */
void main(){
	vec3 worldPos = v + vec3(camPos.x, 0.0, camPos.z);
	gl_Position = mvp * vec4(worldPos, 1.0);
	// Send to almost maximal depth.
	gl_Position.z = gl_Position.w*0.9999;
	oPos = worldPos;
	oSrcPos = worldPos;
	oPrevPos = worldPos;

	
}
