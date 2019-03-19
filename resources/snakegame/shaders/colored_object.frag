#version 330

layout(location = 0) out vec3 fragNormal; ///< Color.
layout(location = 1) out vec4 fragPosId; ///< Color.

in INTERFACE {
	vec3 n;
	vec3 pos;
} In;

uniform int matID;

void main(){
	fragNormal = normalize(In.n)*0.5+0.5;
	fragPosId.xyz = In.pos;
	fragPosId.w = float(matID)/255.0;
}
