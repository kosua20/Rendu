#version 330

in INTERFACE {
	vec3 n; ///< The world-space normal.
} In;

uniform int matID; ///< The material index.

layout(location = 0) out vec3 fragNormal; ///< Normal.
layout(location = 1) out float fragId; ///< Material ID.

/** Outputs the object world-space normal and the material index. */
void main(){
	fragNormal = normalize(In.n)*0.5+0.5;
	fragId = float(matID)/255.0;
}
