#version 400

layout(location = 0) in vec3 v; ///< Position.

uniform mat4 mvp;
uniform vec3 camPos;

out INTERFACE {
	vec3 pos;
} Out;

void main(){
	vec3 worldPos = v + vec3(camPos.x, 0.0, camPos.z);
	gl_Position = mvp * vec4(worldPos, 1.0);
	// Send to almost maximal depth.
	gl_Position.z = gl_Position.w*0.9999;
	Out.pos = worldPos;
}
