
#include "common_pbr.glsl"
#include "forward_lights.glsl"

in INTERFACE {
	vec3 viewSpacePosition; ///< View position.
	vec3 vn; ///< Normal in view space.
} In;

layout(binding = 0) uniform sampler2D albedoTexture; ///< Albedo.
layout(binding = 2) uniform sampler2D effectsTexture; ///< Effects map.
layout(binding = 4) uniform sampler2D brdfPrecalc; ///< Preintegrated BRDF lookup table.
layout(binding = 5) uniform samplerCube textureCubeMap; ///< Background environment cubemap (with preconvoluted versions of increasing roughness in mipmap levels).
layout(binding = 6) uniform sampler2DArray shadowMaps2D; ///< Shadow maps array.
layout(binding = 7) uniform samplerCubeArray shadowMapsCube; ///< Shadow cubemaps array.
layout(binding = 8) uniform sampler2D ssaoTexture; ///< Ambient occlusion.

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
uniform vec2 invScreenSize; ///< Destination size.

uniform int lightsCount; ///< Number of active lights.
/// Store the lights in a continuous buffer (UBO).
layout(std140, binding = 0) uniform Lights {
	GPULight lights[MAX_LIGHTS_COUNT];
};

layout (location = 0) out vec4 fragColor; ///< Ambient contribution.

/** Shade the object, applying lighting. */
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
	float metallic = infos.g;

	// Sample illumination envmap using world space normal and SH pre-computed coefficients.
	vec3 worldN = normalize(vec3(inverseV * vec4(n,0.0)));
	vec3 irradiance = applySH(worldN, shCoeffs);
	// Sample radiance in world space too.
	vec3 worldP = vec3(inverseV * vec4(In.viewSpacePosition, 1.0));
	vec3 worldV = normalize(inverseV[3].xyz - worldP);
	vec3 radiance = radiance(worldN, worldV, worldP, roughness, textureCubeMap, cubemapPos, cubemapCenter, cubemapExtent, cubemapCosSin, maxLod);
	
	// BRDF contributions.
	vec3 diffuse, specular;
	ambientBrdf(baseColor, metallic, roughness, NdotV, brdfPrecalc, diffuse, specular);
	vec2 screenUV = gl_FragCoord.xy * invScreenSize;
	float realtimeAO = textureLod(ssaoTexture, screenUV, 0).r;
	float precomputedAO = infos.b;
	float aoDiffuse = min(realtimeAO, precomputedAO);
	float aoSpecular = approximateSpecularAO(aoDiffuse, NdotV, roughness);

	fragColor = vec4(aoDiffuse * diffuse * irradiance + aoSpecular * specular * radiance, 1.0);

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
		if(!applyLight(lights[lid], In.viewSpacePosition, shadowMapsCube, shadowMaps2D, l, shadowing)){
			continue;
		}
		// Orientation: basic diffuse shadowing.
		float orientation = max(0.0, dot(l,n));
		vec3 specularL = ggx(n, v, l, F0, roughness);
		fragColor.rgb += shadowing * orientation * (diffuseL + specularL) * lights[lid].colorAndBias.rgb;
	}
}
