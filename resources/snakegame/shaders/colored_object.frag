#version 330

layout(location = 0) out vec3 fragNormal; ///< Normal.
layout(location = 1) out float fragId; ///< Material ID.

in INTERFACE {
	vec3 n;
} In;

uniform int matID;

void main(){
	fragNormal = normalize(In.n)*0.5+0.5;
	fragId = float(matID)/255.0;
}
