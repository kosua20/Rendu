
#include "atmosphere.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 0, binding = 0) uniform UniformBlock {
	AtmosphereParameters atmoParams; ///< Custom atmosphere parameters.
	mat4 clipToWorld; ///< Clip-to-world space transformation matrix.
	vec3 viewPos; ///< The position in view space.
	vec3 lightDirection; ///< The light direction in world space.
	float altitude; ///< height above the planet surface.
};

layout(set = 1, binding = 0) uniform sampler2D precomputedScattering; ///< Secondary scattering lookup table.

layout(location = 0) out vec3 fragColor; ///< Atmosphere color.

/** Simulate sky color based on an atmospheric scattering approximate model. */
void main(){
	// Move to -1,1
	vec4 clipVertex = vec4(-1.0+2.0*In.uv, 0.0, 1.0);
	// Then to world space.
	vec3 viewRay = normalize((clipToWorld * clipVertex).xyz);
	// We then move to the ground model space, where the ground is at y=0.
	vec3 groundSpaceViewPos = viewPos + vec3(0.0, altitude, 0.0);
	vec3 atmosphereColor = computeAtmosphereRadiance(groundSpaceViewPos, viewRay, lightDirection, precomputedScattering, atmoParams);
	fragColor = atmosphereColor;
}

