
#include "common_pbr.glsl"
#include "utils.glsl"

layout(set = 2, binding = 0) uniform texture2D albedoTexture; ///< The albedo texture.
layout(set = 2, binding = 1) uniform texture2D normalTexture; ///< The normal texture.
layout(set = 2, binding = 2) uniform texture2D effectsTexture; ///< The effects texture.
layout(set = 2, binding = 3) uniform texture2D depthTexture; ///< The depth texture.
layout(set = 2, binding = 4) uniform texture2D ssaoTexture; ///< The SSAO texture.
layout(set = 2, binding = 5) uniform texture2D brdfPrecalc; ///< Preintegrated BRDF lookup table.
layout(set = 2, binding = 6) uniform textureCube textureCubeMap; ///< Background environment cubemap (with preconvoluted versions of increasing roughness in mipmap levels).

/// SH approximation of the environment probe irradiance (UBO).
layout(std140, set = 3, binding = 0) uniform SHCoeffs {
	vec4 shCoeffs[9];
};

layout(set = 0, binding = 0) uniform UniformBlock {
	mat4 inverseV; ///< The view to world transformation matrix.
	vec4 projectionMatrix; ///< The camera projection matrix.
	vec3 cubemapPos; ///< The cubemap location.
	vec3 cubemapCenter; ///< The cubemap parallax box center.
	vec3 cubemapExtent; ///< The cubemap parallax box half size.
	vec3 cubemapSize; ///< The cubemap effect box half size.
	vec2 cubemapCosSin; ///< The cubemap parallax box orientation (precomputed cos/sin).
	float cubemapFade; ///< The cubemap effect box fade zone size.
	float maxLod; ///< Mip level count for the cubemap.
};

layout(location = 0) out vec4 fragColor; ///< Color.

/** Compute the environment probe ambient lighting contribution on the scene. */
void main(){

	vec2 uv = gl_FragCoord.xy/textureSize(depthTexture, 0).xy;

	float depth = textureLod(sampler2D(depthTexture, sClampNear), uv, 0.0).r;
	vec3 position = positionFromDepth(depth, uv, projectionMatrix);
	vec3 worldP = vec3(inverseV * vec4(position, 1.0));

	// Test if we are inside the effect box or the fade region.
	float weight = probeWeight(worldP, cubemapPos, cubemapSize, cubemapCosSin, cubemapFade);
	if(weight <= 0.0f){
		discard;
	}
	
	vec3 baseColor = textureLod(sampler2D(albedoTexture, sClampNear), uv, 0.0).rgb;
	vec3 infos = textureLod(sampler2D(effectsTexture, sClampNear), uv, 0.0).rgb;
	float roughness = max(0.045, infos.r);
	
	vec3 n = normalize(2.0 * textureLod(sampler2D(normalTexture, sClampLinear), uv, 0.0).rgb - 1.0);
	vec3 v = normalize(-position);
	float NdotV = max(0.0, dot(v, n));

	// Compute AO.
	float precomputedAO = infos.b;
	float realtimeAO = textureLod(sampler2D(ssaoTexture, sClampLinear), uv, 0).r;
	float aoDiffuse = min(realtimeAO, precomputedAO);
	float aoSpecular = approximateSpecularAO(aoDiffuse, NdotV, roughness);

	// Sample illumination envmap using world space normal and SH pre-computed coefficients.
	vec3 worldN = normalize(vec3(inverseV * vec4(n,0.0)));
	vec3 worldV = normalize(inverseV[3].xyz - worldP);
	
	vec3 radiance = radiance(worldN, worldV, worldP, roughness, textureCubeMap, cubemapPos, cubemapCenter, cubemapExtent, cubemapCosSin, maxLod);
	vec3 irradiance = applySH(worldN, shCoeffs);

	// BRDF contributions.
	vec3 diffuse, specular;
	ambientBrdf(baseColor, infos.g, roughness, NdotV, brdfPrecalc, diffuse, specular);
	
	fragColor.rgb = weight * (aoDiffuse * diffuse * irradiance + aoSpecular * specular * radiance);
	
	// Apply weighting.
	fragColor.a = weight;
}
