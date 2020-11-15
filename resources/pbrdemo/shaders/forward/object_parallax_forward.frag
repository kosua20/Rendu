#version 400

#include "common_pbr.glsl"
#include "common_parallax.glsl"
#include "forward_lights.glsl"

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

/// SH approximation of the environment irradiance (UBO).
layout(std140, binding = 1) uniform SHCoeffs {
	vec4 shCoeffs[9];
};

uniform mat4 inverseV; ///< The view to world transformation matrix.
uniform vec3 cubemapPos; ///< The cubemap location
uniform vec3 cubemapCenter; ///< The cubemap parallax box center
uniform vec3 cubemapExtent; ///< The cubemap parallax box half size
uniform vec2 cubemapCosSin; ///< The cubemap parallax box orientation (precomputed cos/sin).
uniform float maxLod; ///< Mip level count for background map.
uniform mat4 p; ///< Projection matrix.

uniform int lightsCount; ///< Number of active lights.
/// Store the lights in a continuous buffer (UBO).
layout(std140, binding = 0) uniform Lights {
	GPULight lights[MAX_LIGHTS_COUNT];
};

layout (location = 0) out vec4 fragAmbient; ///< Ambient contribution.
layout (location = 1) out vec3 fragDirect; ///< Direct lights contribution.
layout (location = 2) out vec3 fragNormal; ///< Surface normal.

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
	fragNormal = n * 0.5 + 0.5;

	vec3 newViewSpacePosition = updateFragmentPosition(localUV, positionShift, In.viewSpacePosition, p, tbn, depthTexture);

	vec3 infos = texture(effectsTexture, localUV).rgb;
	float roughness = max(0.045, infos.r);
	vec3 v = normalize(-newViewSpacePosition);
	float NdotV = max(0.0, dot(v, n));
	float metallic = infos.g;

	// Sample illumination envmap using world space normal and SH pre-computed coefficients.
	vec3 worldN = normalize(vec3(inverseV * vec4(n,0.0)));
	vec3 irradiance = applySH(worldN, shCoeffs);
	// Sample radiance in world space too.
	vec3 worldP = vec3(inverseV * vec4(newViewSpacePosition, 1.0));
	vec3 worldV = normalize(inverseV[3].xyz - worldP);
	vec3 radiance = radiance(worldN, worldV, worldP, roughness, textureCubeMap, cubemapPos, cubemapCenter, cubemapExtent, cubemapCosSin, maxLod);

	// BRDF contributions.
	vec3 diffuse, specular;
	ambientBrdf(baseColor, metallic, roughness, NdotV, brdfPrecalc, diffuse, specular);
	// Store the ambient contribution.
	// Final AO will be computed afterwards, so we only store its precomputed value for now.
	float precomputedAO = infos.b;
	fragAmbient = vec4(diffuse * irradiance + specular * radiance, precomputedAO);

	// Compute F0 (fresnel coeff).
	// Dielectrics have a constant low coeff, metals use the baseColor (ie reflections are tinted).
	vec3 F0 = mix(vec3(0.04), baseColor, metallic);
	// Normalized diffuse contribution. Metallic materials have no diffuse contribution.
	vec3 diffuseL = INV_M_PI * (1.0 - metallic) * baseColor * (1.0 - F0);

	fragDirect = vec3(0.0);
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
		fragDirect += shadowing * orientation * (diffuseL + specularL) * lights[lid].colorAndBias.rgb;
	}
}
