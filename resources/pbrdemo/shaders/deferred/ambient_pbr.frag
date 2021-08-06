
#include "common_pbr.glsl"
#include "utils.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 1, binding = 0) uniform sampler2D albedoTexture; ///< The albedo texture.
layout(set = 1, binding = 1) uniform sampler2D normalTexture; ///< The normal texture.
layout(set = 1, binding = 2) uniform sampler2D effectsTexture; ///< The effects texture.
layout(set = 1, binding = 3) uniform sampler2D depthTexture; ///< The depth texture.
layout(set = 1, binding = 4) uniform sampler2D ssaoTexture; ///< The SSAO texture.
layout(set = 1, binding = 5) uniform sampler2D brdfPrecalc; ///< Preintegrated BRDF lookup table.
layout(set = 1, binding = 6) uniform samplerCube textureCubeMap; ///< Background environment cubemap (with preconvoluted versions of increasing roughness in mipmap levels).

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

	vec4 albedoInfo = texture(albedoTexture,In.uv);
	
	if(albedoInfo.a == 0){
		// If emissive (skybox or object), use directly the albedo color.
		fragColor.rgb = albedoInfo.rgb;
		return;
	}
	
	vec3 baseColor = albedoInfo.rgb;
	vec3 infos = texture(effectsTexture,In.uv).rgb;
	float roughness = max(0.045, infos.r);
	float depth = texture(depthTexture,In.uv).r;
	vec3 position = positionFromDepth(depth, In.uv, projectionMatrix);
	vec3 n = normalize(2.0 * texture(normalTexture,In.uv).rgb - 1.0);
	vec3 v = normalize(-position);
	float NdotV = max(0.0, dot(v, n));

	// Compute AO.
	float precomputedAO = infos.b;
	float realtimeAO = textureLod(ssaoTexture, In.uv, 0).r;
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
