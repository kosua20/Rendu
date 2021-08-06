
#include "atmosphere.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 0, binding = 0) uniform UniformBlock {
	mat4 clipToWorld; ///< Clip-to-world space transformation matrix.
	vec3 viewPos; ///< The position in view space.
	vec3 lightDirection; ///< The light direction in world space.
};

layout(set = 1, binding = 0) uniform sampler2D precomputedScattering; ///< Secondary scattering lookup table.

layout(location = 0) out vec4 fragColor; ///< Atmosphere color.
layout(location = 1) out vec4 fragPos; ///< Atmosphere color.

/** Simulate sky color based on an atmospheric scattering approximate model. */
void main(){
	// Move to -1,1
	vec4 clipVertex = vec4(-1.0+2.0*In.uv, 0.0, 1.0);
	// Then to world space.
	vec3 viewRay = normalize((clipToWorld * clipVertex).xyz);
	// Skip below ground.
	if(viewRay.y < -0.001){
		discard;
	}

	// We then move to the ground model space, where the ground is at y=0.
	vec3 groundSpaceViewPos = viewPos + vec3(0.0, 1.0, 0.0);
	vec3 atmosphereColor = computeAtmosphereRadiance(groundSpaceViewPos, viewRay, lightDirection, precomputedScattering, defaultAtmosphere);

	fragColor.rgb = atmosphereColor;
	fragColor.a = 1.0;
	fragPos.rgb = viewPos + 10000.0 * viewRay;
	fragPos.a = 1.0;
}

