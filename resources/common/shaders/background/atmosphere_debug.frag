#version 400

#include "atmosphere.glsl"

in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In;

uniform mat4 clipToWorld; ///< Clip-to-world space transformation matrix.
uniform vec3 viewPos; ///< The position in view space.
uniform vec3 lightDirection; ///< The light direction in world space.

layout (location = 0) out vec4 fragColor; ///< Atmosphere color.

/** Cheap sky color estimation.
	\param rayOrigin the ray origin
	\param rayDir the ray direction
	\param sunDir the light direction
	\param sunColor the sun color
	\return the estimated radiance
*/
vec3 computeEstimate(vec3 rayOrigin, vec3 rayDir, vec3 sunDir, vec3 sunColor){
	// Check intersection with atmosphere.
	vec2 interTop, interGround;
	bool didHitTop = intersects(rayOrigin, rayDir, atmosphereTopRadius, interTop);
	// If no intersection with the atmosphere, it's the dark void of space.
	if(!didHitTop){
		return vec3(0.0);
	}
	// Now intersect with the planet.
	bool didHitGround = intersects(rayOrigin, rayDir, atmosphereGroundRadius, interGround);
	
	// The sun itself if we're looking at it.
	vec3 sunRadiance = vec3(0.0);
	bool didHitGroundForward = didHitGround && interGround.y > 0;
	if(!didHitGroundForward && dot(rayDir, sunDir) > sunAngularRadiusCos){
		sunRadiance = sunColor / (M_PI * sunAngularRadius * sunAngularRadius);
	}

	float sunHeight = clamp(sunDir.y, 0.0, 1.0);
	float rayHeight = clamp(rayDir.y, 0.0, 1.0);
	// Approximate sky color with gradients wrt the sun height (time of day) and ray height (region of the hemisphere).
	vec3 bottomSkyColor = mix(vec3(0.831, 0.439, 0.141), vec3(0.615, 0.788, 0.956), sunHeight);
	vec3 topSkyColor = mix(vec3(0.333, 0.333, 0.317), vec3(0.439, 0.639, 0.839), sunHeight);
	vec3 groundColor = mix(vec3(0.018, 0.005, 0.0), vec3(0.011, 0.01, 0.008), sunHeight);
	return sunRadiance + (didHitGroundForward ? groundColor : mix(bottomSkyColor, topSkyColor, rayHeight));
}

/** Simulate sky color based on an atmospheric scattering approximate model. */
void main(){
	// Move to -1,1
	vec4 clipVertex = vec4(-1.0+2.0*In.uv, 0.0, 1.0);
	// Then to world space.
	vec3 viewRay = normalize((clipToWorld * clipVertex).xyz);
	// We then move to the planet model space, where its center is in (0,0,0).
	vec3 planetSpaceViewPos = viewPos + vec3(0,6371e3,0) + vec3(0.0,1.0,0.0);
	vec3 atmosphereColor = computeEstimate(planetSpaceViewPos, viewRay, lightDirection, defaultSunColor);
	fragColor.rgb = atmosphereColor;
	fragColor.a = 1.0;
}

