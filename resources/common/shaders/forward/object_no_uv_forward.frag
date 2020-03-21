#version 330

#include "common_ambient_pbr.glsl"

in INTERFACE {
	vec3 viewSpacePosition; ///< View position.
	vec3 vn; ///< Normal in view space.
} In;

layout(binding = 0) uniform sampler2D albedoTexture; ///< Albedo.
layout(binding = 2) uniform sampler2D effectsTexture; ///< Effects map.

layout(binding = 3) uniform sampler2D brdfPrecalc; ///< Preintegrated BRDF lookup table.
layout(binding = 4) uniform samplerCube textureCubeMap; ///< Background environment cubemap (with preconvoluted versions of increasing roughness in mipmap levels).

uniform vec3 shCoeffs[9]; ///< SH approximation of the environment irradiance.
uniform mat4 inverseV; ///< The view to world transformation matrix.
uniform float maxLod; ///< Mip level count for background map.

layout (location = 0) out vec3 fragColor; ///< Color.


/** Transfer albedo and effects along with the material ID, and output the final normal 
	(combining geometry normal and normal map) in view space. */
void main(){
	const vec2 defaultUV = vec2(0.5);
	vec4 albedoInfos = texture(albedoTexture, defaultUV);
	if(albedoInfos.a <= 0.01){
		discard;
	}
	vec3 baseColor = albedoInfos.rgb;
	
	vec3 n = (gl_FrontFacing ? 1.0 : -1.0) * normalize(In.vn);

	vec3 infos = texture(effectsTexture, defaultUV).rgb;
	float roughness = max(0.045, infos.r);
	vec3 v = normalize(-In.viewSpacePosition);
	float NdotV = max(0.0, dot(v, n));

	// Compute AO.
	float precomputedAO = infos.b;
	float realtimeAO = 1.0; // No AO for now.
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
