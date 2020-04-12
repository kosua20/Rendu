#version 400

#include "common_pbr.glsl"
#include "common_parallax.glsl"
#include "common_lights.glsl"

in INTERFACE {
    mat3 tbn; ///< Normal to view matrix.
	vec3 tangentSpacePosition; ///< Tangent space position.
	vec3 viewSpacePosition; ///< View space position.
	vec2 uv; ///< UV coordinates.
} In ;

layout(binding = 0) uniform sampler2D albedoTexture; ///< Albedo.
layout(binding = 1) uniform sampler2D normalTexture; ///< Normal map.
layout(binding = 2) uniform sampler2D effectsTexture; ///< Effects map.
layout(binding = 3) uniform sampler2D depthTexture; ///< Effects map.
layout(binding = 4) uniform sampler2D brdfPrecalc; ///< Preintegrated BRDF lookup table.
layout(binding = 5) uniform samplerCube textureCubeMap; ///< Background environment cubemap (with preconvoluted versions of increasing roughness in mipmap levels).
layout(binding = 6) uniform sampler2DArray shadowMaps2D; ///< Shadow maps array.
layout(binding = 7) uniform samplerCubeArray shadowMapsCube; ///< Shadow cubemaps array.

uniform vec3 shCoeffs[9]; ///< SH approximation of the environment irradiance.
uniform mat4 inverseV; ///< The view to world transformation matrix.
uniform float maxLod; ///< Mip level count for background map.
uniform mat4 p; ///< Projection matrix.

uniform int lightsCount; ///< Number of active lights.
/// Store the lights in a continuous buffer (UBO).
layout(std140) uniform Lights {
	GPULight lights[MAX_LIGHTS_COUNT];
};

layout (location = 0) out vec3 fragColor; ///< Color.

/** Shade the object, applying parallax mapping. */
void main(){
	
	vec2 localUV = In.uv;
	vec2 positionShift;
	
	// Compute the new uvs, and use them for the remaining steps.
	vec3 vTangentDir = normalize(- In.tangentSpacePosition);
	localUV = parallax(localUV, vTangentDir, depthTexture, positionShift);
	// If UV are outside the texture ([0,1]), we discard the fragment.
	if(localUV.x > 1.0 || localUV.y  > 1.0 || localUV.x < 0.0 || localUV.y < 0.0){
		discard;
	}

	vec4 albedoInfos = texture(albedoTexture, localUV);
	if(albedoInfos.a <= 0.01){
		discard;
	}
	vec3 baseColor = albedoInfos.rgb;
	
	// Flip the up of the local frame for back facing fragments.
	mat3 tbn = In.tbn;
	tbn[2] *= (gl_FrontFacing ? 1.0 : -1.0);
	// Compute the normal at the fragment using the tangent space matrix and the normal read in the normal map.
	vec3 n = texture(normalTexture, localUV).rgb ;
	n = normalize(n * 2.0 - 1.0);
	n = normalize(tbn * n);

	vec3 newViewSpacePosition = updateFragmentPosition(localUV, positionShift, In.viewSpacePosition, p, tbn, depthTexture);

	vec3 infos = texture(effectsTexture, localUV).rgb;
	float roughness = max(0.045, infos.r);
	vec3 v = normalize(-newViewSpacePosition);
	float NdotV = max(0.0, dot(v, n));
	float metallic = infos.g;

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
	ambientBrdf(baseColor, metallic, roughness, NdotV, brdfPrecalc, diffuse, specular);
	
	fragColor = aoDiffuse * diffuse * irradiance + aoSpecular * specular * radiance;

	// Compute F0 (fresnel coeff).
	// Dielectrics have a constant low coeff, metals use the baseColor (ie reflections are tinted).
	vec3 F0 = mix(vec3(0.04), baseColor, metallic);
	// Normalized diffuse contribution. Metallic materials have no diffuse contribution.
	vec3 diffuseL = INV_M_PI * (1.0 - metallic) * baseColor * (1.0 - F0);

	for(int lid = 0; lid < MAX_LIGHTS_COUNT; ++lid){
		if(lid >= lightsCount){
			break;
		}
		float shadowing;
		vec3 l;
		if(!applyLight(lights[lid], newViewSpacePosition, shadowMapsCube, shadowMaps2D, l, shadowing)){
			continue;
		}
		// Orientation: basic diffuse shadowing.
		float orientation = max(0.0, dot(l,n));
		vec3 specularL = ggx(n, v, l, F0, roughness);
		fragColor += shadowing * orientation * (diffuseL + specularL) * lights[lid].colorAndBias.rgb;
	}
}
