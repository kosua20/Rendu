#version 330

#include "common_pbr.glsl"

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

#define SAMPLES_COUNT 16u


/** Return the (pre-convolved) radiance for a given normal, view direction and
	material parameters.
	\param n the surface normal
	\param v the view direction
	\param roughness the surface roughness
	\return the radiance value
	*/
vec3 radiance(vec3 n, vec3 v, float roughness){
	// Compute local frame.
	vec3 r = -reflect(v,n);
	r = normalize((inverseV * vec4(r, 0.0)).xyz);
	
	vec3 specularColor = textureLod(textureCubeMap, r, maxLod * roughness).rgb;
	return specularColor;
}

/** Evaluate the ambient irradiance (as SH coefficients) in a given direction. 
	\param wn the direction (normalized)
	\return the ambient irradiance
	*/
vec3 applySH(vec3 wn){
	return (shCoeffs[7] * wn.z + shCoeffs[4]  * wn.y + shCoeffs[8]  * wn.x + shCoeffs[3]) * wn.x +
		   (shCoeffs[5] * wn.z - shCoeffs[8]  * wn.y + shCoeffs[1]) * wn.y +
		   (shCoeffs[6] * wn.z + shCoeffs[2]) * wn.z +
		    shCoeffs[0];
}

/** Estimate specular ambient occlusion. Based on "Moving Frostbite to Physically Based Rendering".
	\param diffuseAO diffuse visbility factor
	\param NdotV visibility/normal angle
	\param roughness linear material roughness
	\return the estimated specular visibility
*/
float approximateSpecularAO(float diffuseAO, float NdotV, float roughness){
	float specAO = pow(NdotV + diffuseAO, exp2(-16.0 * roughness - 1.0));
	return clamp(specAO - 1.0 + diffuseAO, 0.0, 1.0);
}

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
	vec3 irradiance = applySH(worldNormal);
	vec3 radiance = radiance(n, v, roughness);
	
	// BRDF contributions.
	// Compute F0 (fresnel coeff).
	// Dielectrics have a constant low coeff, metals use the baseColor (ie reflections are tinted).
	vec3 F0 = mix(vec3(0.04), baseColor, metallic);
	// Adjust Fresnel based on roughness.
	vec3 Fr = max(vec3(1.0 - roughness), F0) - F0;
    vec3 Fs = F0 + Fr * pow(1.0 - NdotV, 5.0);
	// Specular single scattering contribution (preintegrated).
	vec2 brdfParams = texture(brdfPrecalc, vec2(NdotV, roughness)).rg;
	vec3 specular = (brdfParams.x * Fs + brdfParams.y);
	
	// Account for multiple scattering.
	// Based on A Multiple-Scattering Microfacet Model for Real-Time Image-based Lighting, C. J. Fdez-Agüera, JCGT, 2019.
    float scatter = (1.0 - (brdfParams.x + brdfParams.y));
    vec3 Favg = F0 + (1.0 - F0) / 21.0;
    vec3 multi = scatter * specular * Favg / (1.0 - Favg * scatter);
	// Diffuse contribution. Metallic materials have no diffuse contribution.
	vec3 single = (1.0 - metallic) * baseColor * (1.0 - F0);
	vec3 diffuse = single * (1.0 - specular - multi) + multi;
	
	fragColor = aoDiffuse * diffuse * irradiance + aoSpecular * specular * radiance;
}
