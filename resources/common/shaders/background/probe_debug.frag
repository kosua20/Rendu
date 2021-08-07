#include "utils.glsl"
#include "common_pbr.glsl"

#define PROBE_SHCOEFFS 0
#define PROBE_RADIANCE 1

layout(location = 0) in INTERFACE {
	vec4 pos; ///< World space position.
    vec4 nor; ///< Normal in world space.
} In ;

layout(set = 1, binding = 0) uniform samplerCube texture0; ///< Cubemap color.

/// SH approximation of the environment irradiance.
layout(std140, set = 2, binding = 0) uniform SHCoeffs {
	vec4 shCoeffs[9];
};

layout(set = 0, binding = 0) uniform UniformBlock {
	vec3 camPos; ///< Camera position in world space.
	float lod; ///< Level to fetch.
	int mode; ///< Display mod.
};

layout(location = 0) out vec4 fragColor; ///< Color.

/** Use the normalized position to read in the cube map. */
void main(){
	vec3 n = normalize(In.nor.xyz);
	if(mode == PROBE_SHCOEFFS){
		fragColor = vec4(applySH(n, shCoeffs), 1.0);
	} else {
		vec3 v = normalize(In.pos.xyz - camPos);
		vec3 r = reflect(v, n);
		fragColor = textureLod(texture0, toCube(r), lod);
	}

}
