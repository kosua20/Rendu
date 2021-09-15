
#include "common_pbr.glsl"
#include "utils.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 1, binding = 0) uniform texture2D albedoTexture; ///< The albedo texture.
layout(set = 1, binding = 1) uniform texture2D normalTexture; ///< The normal texture.
layout(set = 1, binding = 2) uniform texture2D effectsTexture; ///< The effects texture.
layout(set = 1, binding = 3) uniform texture2D depthTexture; ///< The depth texture.
layout(set = 1, binding = 4) uniform texture2D ssaoTexture; ///< The SSAO texture.
layout(set = 1, binding = 5) uniform texture2D brdfPrecalc; ///< Preintegrated BRDF lookup table.
layout(set = 1, binding = 6) uniform textureCube textureCubeMap; ///< Background environment cubemap (with preconvoluted versions of increasing roughness in mipmap levels).

/// SH approximation of the environment irradiance (UBO).
layout(std140, set = 2, binding = 0) uniform SHCoeffs {
	vec4 shCoeffs[9];
};

layout(set = 0, binding = 0) uniform UniformBlock {
	mat4 inverseV; ///< The view to world transformation matrix.
	vec4 projectionMatrix; ///< The camera projection matrix.
	vec3 cubemapPos; ///< The cubemap location
	vec3 cubemapCenter; ///< The cubemap parallax box center
	vec3 cubemapExtent; ///< The cubemap parallax box half size
	vec2 cubemapCosSin; ///< The cubemap parallax box orientation (precomputed cos/sin).
	float maxLod; ///< Mip level count for background map.
};

layout(location = 0) out vec4 fragColor; ///< Color.

/** Compute the environment ambient lighting contribution on the scene. */
void main(){
	fragColor.a = 1.0;

	vec4 albedoInfo = texture(sampler2D(albedoTexture, sClampNear),In.uv);
	
	if(albedoInfo.a == 0){
		// If emissive (skybox or object), use directly the albedo color.
		fragColor.rgb = albedoInfo.rgb;
		return;
	}
	
	vec3 baseColor = albedoInfo.rgb;
	vec3 infos = texture(sampler2D(effectsTexture, sClampNear),In.uv).rgb;
	float roughness = max(0.045, infos.r);
	float depth = texture(sampler2D(depthTexture, sClampNear),In.uv).r;
	vec3 position = positionFromDepth(depth, In.uv, projectionMatrix);
	vec3 n = normalize(2.0 * texture(sampler2D(normalTexture, sClampLinear),In.uv).rgb - 1.0);
	vec3 v = normalize(-position);
	float NdotV = max(0.0, dot(v, n));

	// Compute AO.
	float precomputedAO = infos.b;
	float realtimeAO = textureLod(sampler2D(ssaoTexture, sClampLinear), In.uv, 0).r;
	float aoDiffuse = min(realtimeAO, precomputedAO);
	float aoSpecular = approximateSpecularAO(aoDiffuse, NdotV, roughness);

	// Sample illumination envmap using world space normal and SH pre-computed coefficients.
	vec3 worldN = normalize(vec3(inverseV * vec4(n,0.0)));
	vec3 irradiance = applySH(worldN, shCoeffs);
	// Sample radiance in world space too.
	vec3 worldP = vec3(inverseV * vec4(position, 1.0));
	vec3 worldV = normalize(inverseV[3].xyz - worldP);
	vec3 radiance = radiance(worldN, worldV, worldP, roughness, textureCubeMap, cubemapPos, cubemapCenter, cubemapExtent, cubemapCosSin, maxLod);
	
	// BRDF contributions.
	vec3 diffuse, specular;
	ambientBrdf(baseColor, infos.g, roughness, NdotV, brdfPrecalc, diffuse, specular);
	
	fragColor.rgb = aoDiffuse * diffuse * irradiance + aoSpecular * specular * radiance;
}
