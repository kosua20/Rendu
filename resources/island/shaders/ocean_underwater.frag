#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 0, binding = 0) uniform UniformBlock {
	vec3 camPos; ///< Camera world position.
	vec2 invTargetSize; ///< Inverse framebuffer size.
};

layout(set = 2, binding = 1) uniform texture2D terrainColor; ///< Shaded terrain (with caustics)
layout(set = 2, binding = 2) uniform texture2D terrainPos; ///< Terrain world position map.
layout(set = 2, binding = 3) uniform texture2D terrainColorBlur; ///< Blurred shaded terrain.
layout(set = 2, binding = 4) uniform texture2D absorpScatterLUT; ///< Absorption/scattering look-up table.

layout (location = 0) out vec4 fragColor; ///< Underwater appearance.
layout (location = 1) out vec4 fragPosUnused; ///< Position (unused).

/** Apply scattering and absorption on top of the rendered terrain, with depth-aware effects. */
void main(){
	vec3 oceanFloorPos = texelFetch(sampler2D(terrainPos, sClampNear), ivec2(gl_FragCoord.xy), 0).rgb;
	float distPos = distance(oceanFloorPos, camPos);
	// Put the floor at 1.0 unit below approx.
	float scalingDist = 1.0/3.0;
	float distUnderWater = clamp(distPos * scalingDist, 0.0, 1.0);

	// Blend blurred version of the floor in deeper regions.
	vec2 screenUV = (gl_FragCoord.xy)*invTargetSize;
	vec3 oceanFloor = textureLod(sampler2D(terrainColor, sClampLinear), screenUV, 0.0).rgb;
	vec3 oceanFloorBlur = textureLod(sampler2D(terrainColorBlur, sClampLinear), screenUV, 0.0).rgb;
	vec3 floorColor = mix(oceanFloor, oceanFloorBlur, distUnderWater);

	// Lookup absorption and scattering values for the given distance under water.
	vec3 absorp = textureLod(sampler2D(absorpScatterLUT, sClampLinear), vec2(distUnderWater, 0.25), 0.0).rgb;
	vec3 scatter = textureLod(sampler2D(absorpScatterLUT, sClampLinear), vec2(distUnderWater, 0.75), 0.0).rgb;
	vec3 baseColor = absorp * mix(floorColor, scatter, distUnderWater);

	fragColor = vec4(baseColor,1.0);
	fragPosUnused = vec4(1.0);
}
