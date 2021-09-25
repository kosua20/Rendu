
#include "common_pbr.glsl"

layout(location = 0) in INTERFACE {
	vec3 pos; ///< Position in model space.
} In ;

/// SH approximation of the environment irradiance (UBO).
layout(std140, set = 3, binding = 0) uniform SHCoeffs {
	vec4 shCoeffs[9];
};

layout(location = 0) out vec3 fragColor; ///< Color.

/** Compute the environment ambient lighting contribution on the scene. */
void main(){
	vec3 envLighting = applySH(normalize(In.pos), shCoeffs);
	fragColor = envLighting;
}
