
#include "common_pbr.glsl"
#include "utils.glsl"

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

/// SH approximation of the environment irradiance (UBO).
layout(std140, binding = 0) uniform SHCoeffs {
	vec4 shCoeffs[9];
};
uniform mat4 inverseV; ///< The view to world transformation matrix.
uniform vec4 projectionMatrix; ///< The camera projection matrix.

uniform vec3 cubemapPos; ///< The cubemap location
uniform vec3 cubemapCenter; ///< The cubemap parallax box center
uniform vec3 cubemapExtent; ///< The cubemap parallax box half size
uniform vec2 cubemapCosSin; ///< The cubemap parallax box orientation (precomputed cos/sin).
uniform float maxLod; ///< Mip level count for background map.

layout(location = 0) out vec3 fragColor; ///< Color.

/** Compute the environment ambient lighting contribution on the scene. */
void main(){
	
	vec4 albedoInfo = texture(albedoTexture,In.uv);
	
	if(albedoInfo.a == 0){
		// If emissive (skybox or object), use directly the albedo color.
		fragColor = albedoInfo.rgb;
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
	float realtimeAO = texture(ssaoTexture, In.uv).r;
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
	
	fragColor = aoDiffuse * diffuse * irradiance + aoSpecular * specular * radiance;
}
