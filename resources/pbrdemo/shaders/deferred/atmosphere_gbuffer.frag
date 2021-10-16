#include "materials.glsl"
#include "atmosphere.glsl"
#include "utils.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv;  ///< Texture coordinates.
} In ;

layout(set = 0, binding = 0) uniform UniformBlock {
	mat4 clipToWorld; ///< Clip-to-world space transformation matrix.
	vec3 viewPos; ///< The position in view space.
	vec3 lightDirection; ///< The light direction in world space.
};

layout(set = 2, binding = 0) uniform texture2D precomputedScattering; ///< Secondary scattering lookup table.

layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec4 fragNormal; ///< View space normal.
layout (location = 2) out vec4 fragEffects; ///< Effects.

/** Simulate sky color based on an atmospheric scattering approximate model. */
void main(){
	// Move to -1,1
	vec4 clipVertex = vec4(-1.0+2.0*In.uv, 0.0, 1.0);
	// Then to world space.
	vec3 viewRay = normalize((clipToWorld * clipVertex).xyz);
	// We then move to the ground model space, where the ground is at y=0.
	vec3 groundSpaceViewPos = viewPos + vec3(0.0, 1.0, 0.0);
	vec3 atmosphereColor = computeAtmosphereRadiance(groundSpaceViewPos, viewRay, lightDirection, precomputedScattering, defaultAtmosphere);
	// Normalize emissive color, store intensity in effects texture.
	float m = max(max(atmosphereColor.r, 0.001), max(atmosphereColor.g, atmosphereColor.b));
	fragColor.rgb = atmosphereColor/m;
	fragColor.a = encodeMaterial(MATERIAL_EMISSIVE);
	fragNormal.rgb = vec3(0.5);
	fragNormal.a = 0.0;
	fragEffects = floatToVec4(m);
}

