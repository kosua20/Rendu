#version 400

in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

uniform vec3 camPos; ///< Camera world position.
uniform vec2 invTargetSize; ///< Inverse framebuffer size.

layout(binding = 1) uniform sampler2D terrainColor; ///< Shaded terrain (with caustics)
layout(binding = 2) uniform sampler2D terrainPos; ///< Terrain world position map.
layout(binding = 3) uniform sampler2D terrainColorBlur; ///< Blurred shaded terrain.
layout(binding = 4) uniform sampler2D absorpScatterLUT; ///< Absorption/scattering look-up table.

layout (location = 0) out vec4 fragColor; ///< Underwater appearance.

/** Apply scattering and absorption on top of the rendered terrain, with depth-aware effects. */
void main(){
	vec3 oceanFloorPos = texelFetch(terrainPos, ivec2(gl_FragCoord.xy), 0).rgb;
	float distPos = distance(oceanFloorPos, camPos);
	// Put the floor at 1.0 unit below approx.
	float scalingDist = 1.0/3.0;
	float distUnderWater = clamp(distPos * scalingDist, 0.0, 1.0);

	// Blend blurred version of the floor in deeper regions.
	vec2 screenUV = (gl_FragCoord.xy)*invTargetSize;
	vec3 oceanFloor = textureLod(terrainColor, screenUV, 0.0).rgb;
	vec3 oceanFloorBlur = textureLod(terrainColorBlur, screenUV, 0.0).rgb;
	vec3 floorColor = mix(oceanFloor, oceanFloorBlur, distUnderWater);

	// Lookup absorption and scattering values for the given distance under water.
	vec3 absorp = textureLod(absorpScatterLUT, vec2(distUnderWater, 0.75), 0.0).rgb;
	vec3 scatter = textureLod(absorpScatterLUT, vec2(distUnderWater, 0.25), 0.0).rgb;
	vec3 baseColor = absorp * mix(floorColor, scatter, distUnderWater);

	fragColor = vec4(baseColor,1.0);
}
