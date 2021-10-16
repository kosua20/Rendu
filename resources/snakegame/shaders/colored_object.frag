#include "utils.glsl"

layout(location = 0) in INTERFACE {
	vec3 n; ///< The world-space normal.
} In;

layout(set = 0, binding = 0) uniform UniformBlock {
	int matID; ///< The material index.
};

layout(location = 0) out vec4 fragNormal; ///< Normal.
layout(location = 1) out float fragId; ///< Material ID.

/** Outputs the object world-space normal and the material index. */
void main(){
	fragNormal.rg = encodeNormal(normalize(In.n));
	fragNormal.ba = vec2(1.0);
	fragId = float(matID)/255.0;
}
