#version 330

#include "common_pbr.glsl"
#include "common_ambient_pbr.glsl"

in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;


layout(binding = 0) uniform sampler2D albedoTexture; ///< The albedo texture.
layout(binding = 1) uniform sampler2D normalTexture; ///< The normal texture.
layout(binding = 2) uniform sampler2D effectsTexture; ///< The effects texture.
layout(binding = 3) uniform sampler2D depthTexture; ///< The depth texture.
layout(binding = 4) uniform sampler2D ssaoTexture; ///< The SSAO texture.
layout(binding = 5) uniform sampler2D brdfPrecalc; ///< Preintegrated BRDF lookup table.
layout(binding = 6) uniform samplerCube textureCubeMap; ///< Background environment cubemap (with preconvoluted versions of increasing roughness in mipmap levels).

uniform vec3 shCoeffs[9]; ///< SH approximation of the environment irradiance.
uniform mat4 inverseV; ///< The view to world transformation matrix.
uniform vec4 projectionMatrix; ///< The camera projection matrix.
uniform float maxLod; ///< Mip level count for background map.

layout(location = 0) out vec3 fragColor; ///< Color.

/** Compute the environment ambient lighting contribution on the scene. */
void main(){
	
	vec4 albedoInfo = texture(albedoTexture,In.uv);
	
	if(albedoInfo.a == 0){
		// If background (skybox), use directly the diffuse color.
		fragColor = albedoInfo.rgb;
		return;
	}
	
	vec3 baseColor = albedoInfo.rgb;
	vec3 infos = texture(effectsTexture,In.uv).rgb;
	float roughness = max(0.045, infos.r);
	float metallic = infos.g;
	float depth = texture(depthTexture,In.uv).r;
	vec3 position = positionFromDepth(depth, In.uv, projectionMatrix);
	vec3 n = normalize(2.0 * texture(normalTexture,In.uv).rgb - 1.0);
	vec3 v = normalize(-position);
	float NdotV = max(0.0, dot(v, n));

	// Compute AO.
	float precomputedAO = infos.b;
	float realtimeAO = texture(ssaoTexture, In.uv).r;
	float aoDiffuse = min(realtimeAO, precomputedAO);
	float aoSpecular = approximateSpecularAO(aoDiffuse, NdotV, roughness);

	// Sample illumination envmap using world space normal and SH pre-computed coefficients.
	vec3 worldNormal = normalize(vec3(inverseV * vec4(n,0.0)));
	vec3 irradiance = applySH(worldNormal, shCoeffs);
	vec3 radiance = radiance(n, v, roughness, inverseV, textureCubeMap, maxLod);
	
	// BRDF contributions.
	vec3 diffuse, specular;
	ambientBrdf(baseColor, infos.g, roughness, NdotV, brdfPrecalc, diffuse, specular);
	
	fragColor = aoDiffuse * diffuse * irradiance + aoSpecular * specular * radiance;
}
