
#include "atmosphere.glsl"

#define MATERIAL_ID 0 ///< The material ID.

layout(location = 0) in INTERFACE {
	vec2 uv;  ///< Texture coordinates.
} In ;

layout(set = 0, binding = 0) uniform UniformBlock {
	mat4 clipToWorld; ///< Clip-to-world space transformation matrix.
	vec3 viewPos; ///< The position in view space.
	vec3 lightDirection; ///< The light direction in world space.
};

layout(set = 1, binding = 0) uniform texture2D precomputedScattering; ///< Secondary scattering lookup table.

layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec3 fragNormal; ///< View space normal.
layout (location = 2) out vec3 fragEffects; ///< Effects.

/** Simulate sky color based on an atmospheric scattering approximate model. */
void main(){
	// Move to -1,1
	vec4 clipVertex = vec4(-1.0+2.0*In.uv, 0.0, 1.0);
	// Then to world space.
	vec3 viewRay = normalize((clipToWorld * clipVertex).xyz);
	// We then move to the ground model space, where the ground is at y=0.
	vec3 groundSpaceViewPos = viewPos + vec3(0.0, 1.0, 0.0);
	vec3 atmosphereColor = computeAtmosphereRadiance(groundSpaceViewPos, viewRay, lightDirection, precomputedScattering, defaultAtmosphere);

	fragColor.rgb = atmosphereColor;
	fragColor.a = MATERIAL_ID;
	fragNormal = vec3(0.5);
	fragEffects = vec3(0.0);
}

