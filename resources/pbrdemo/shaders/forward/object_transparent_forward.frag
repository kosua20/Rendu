
#include "common_pbr.glsl"
#include "forward_lights.glsl"

layout(location = 0) in INTERFACE {
	mat4 tbn; ///< Normal to view matrix.
	vec4 viewSpacePosition; ///< View space position.
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D albedoTexture; ///< Albedo.
layout(set = 2, binding = 1) uniform texture2D normalTexture; ///< Normal map.
layout(set = 2, binding = 2) uniform texture2D effectsTexture; ///< Effects map.
layout(set = 2, binding = 4) uniform texture2D brdfPrecalc; ///< Preintegrated BRDF lookup table.
layout(set = 2, binding = 5) uniform textureCube textureProbes[MAX_PROBES_COUNT]; ///< Background environment cubemaps (with preconvoluted versions of increasing roughness in mipmap levels).
layout(set = 2, binding = 6) uniform texture2DArray shadowMaps2D; ///< Shadow maps array.
layout(set = 2, binding = 7) uniform textureCubeArray shadowMapsCube; ///< Shadow cubemaps array.

layout(set = 0, binding = 0) uniform UniformBlock {
	mat4 inverseV; ///< The view to world transformation matrix.
	vec2 invScreenSize; ///< Destination size.
	int lightsCount; ///< Number of active lights.
	int probesCount; ///< Number of active envmaps.
	bool hasUV; ///< Does the mesh have UV coordinates.
};

/// Store the lights in a continuous buffer (UBO).
layout(std140, set = 3, binding = 0) uniform Lights {
	GPULight lights[MAX_LIGHTS_COUNT];
};

/// Store the probes in a continuous buffer (UBO).
layout(std140, set = 3, binding = 1) uniform Probes {
	GPUProbe probes[MAX_PROBES_COUNT];
};

///SH approximations of the environment probes irradiance (UBO). 
layout(std140, set = 3, binding = 2) uniform SHCoeffs {
	vec4 coeffs[9];
} probesSH[MAX_PROBES_COUNT];

layout (location = 0) out vec4 fragColor; ///< Shading result.

/** Shade the object, applying lighting. */
void main(){

	vec4 albedoInfos = texture(sampler2D(albedoTexture, sRepeatLinearLinear), In.uv);
	// No clipping, specular component is always present.
	vec3 baseColor = albedoInfos.rgb;

	// Flip the up of the local frame for back facing fragments.
	mat3 tbn = mat3(In.tbn);
	tbn[2] *= (gl_FrontFacing ? 1.0 : -1.0);
	// Compute the normal at the fragment using the tangent space matrix and the normal read in the normal map.
	// If we dont have UV, use the geometric normal.
	vec3 n;
	if(hasUV){
		n = texture(sampler2D(normalTexture, sRepeatLinearLinear), In.uv).rgb ;
		n = normalize(n * 2.0 - 1.0);
		n = normalize(tbn * n);
	} else {
		n = normalize(tbn[2]);
	}

	vec3 infos = texture(sampler2D(effectsTexture, sRepeatLinearLinear), In.uv).rgb;
	float roughness = max(0.045, infos.r);
	vec3 v = normalize(-In.viewSpacePosition.xyz);
	float NdotV = max(0.0, dot(v, n));
	float metallic = infos.g;

	// Sample illumination envmap using world space normal and SH pre-computed coefficients.
	vec3 worldN = normalize(vec3(inverseV * vec4(n,0.0)));
	vec3 worldP = vec3(inverseV * vec4(In.viewSpacePosition.xyz, 1.0));
	vec3 worldV = normalize(inverseV[3].xyz - worldP);
	
	// Accumulate envmaps contributions.
	vec3 irradiance = vec3(0.0);
	vec4 radiance = vec4(0.0);
	for(int pid = 0; pid < MAX_PROBES_COUNT; ++pid){
		if(pid >= probesCount){
			break;
		}
		// Sample radiance in world space too.
		vec4 radianceAndWeight = applyProbe(probes[pid], worldN, worldV, worldP, roughness, textureProbes[pid]);
		radiance += radianceAndWeight;
		irradiance += radianceAndWeight.w * applySH(worldN, probesSH[pid].coeffs);
	}
	if(radiance.w != 0.0){
		radiance /= radiance.w;
		irradiance /= radiance.w;
	}

	// BRDF contributions.
	vec3 diffuse, specular;
	ambientBrdf(baseColor, metallic, roughness, NdotV, brdfPrecalc, diffuse, specular);

	float precomputedAO = infos.b;
	float aoDiffuse = precomputedAO;
	float aoSpecular = approximateSpecularAO(aoDiffuse, NdotV, roughness);

	fragColor = vec4(albedoInfos.a * aoDiffuse * diffuse * irradiance + aoSpecular * specular * radiance.rgb, albedoInfos.a);

	// Compute F0 (fresnel coeff).
	// Dielectrics have a constant low coeff, metals use the baseColor (ie reflections are tinted).
	vec3 F0 = mix(vec3(0.04), baseColor, metallic);
	// Normalized diffuse contribution. Metallic materials have no diffuse contribution.
	vec3 diffuseL = albedoInfos.a * INV_M_PI * (1.0 - metallic) * baseColor * (1.0 - F0);

	for(int lid = 0; lid < MAX_LIGHTS_COUNT; ++lid){
		if(lid >= lightsCount){
			break;
		}
		float shadowing;
		vec3 l;
		if(!applyLight(lights[lid], In.viewSpacePosition.xyz, shadowMapsCube, shadowMaps2D, l, shadowing)){
			continue;
		}
		// Orientation: basic diffuse shadowing.
		float orientation = max(0.0, dot(l,n));
		vec3 specularL = ggx(n, v, l, F0, roughness);
		fragColor.rgb += shadowing * orientation * (diffuseL + specularL) * lights[lid].colorAndBias.rgb;
	}
}
