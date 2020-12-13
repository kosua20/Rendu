
#include "common_pbr.glsl"

#define PROBE_SHCOEFFS 0
#define PROBE_RADIANCE 1

in INTERFACE {
	vec3 pos; ///< World space position.
    vec3 nor; ///< Normal in world space.
} In ;

layout(binding = 0) uniform samplerCube texture0; ///< Cubemap color.
/// SH approximation of the environment irradiance.
layout(std140, binding = 0) uniform SHCoeffs {
	vec4 shCoeffs[9];
};
uniform vec3 camPos; ///< Camera position in world space.
uniform int mode = 1; ///< Display mod.
uniform float lod = 0.0; ///< Level to fetch.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Use the normalized position to read in the cube map. */
void main(){
	vec3 n = normalize(In.nor);
	if(mode == PROBE_SHCOEFFS){
		fragColor = vec4(applySH(n, shCoeffs), 1.0);
	} else {
		vec3 v = normalize(In.pos - camPos);
		vec3 r = reflect(v, n);
		fragColor = textureLod(texture0, r, lod);
	}

}
