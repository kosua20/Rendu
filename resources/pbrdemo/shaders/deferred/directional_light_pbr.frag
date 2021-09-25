
#include "common_pbr.glsl"
#include "shadow_maps.glsl"
#include "utils.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

// Uniforms
layout(set = 2, binding = 0) uniform texture2D albedoTexture; ///< Albedo.
layout(set = 2, binding = 1) uniform texture2D normalTexture; ///< Normal.
layout(set = 2, binding = 2) uniform texture2D depthTexture; ///< Depth.
layout(set = 2, binding = 3) uniform texture2D effectsTexture; ///< Effects.
layout(set = 2, binding = 4) uniform texture2DArray shadowMap; ///< Shadow map.

layout(set = 0, binding = 0) uniform UniformBlock {
	mat4 viewToLight; ///< View to light space matrix.
	vec4 projectionMatrix; ///< Camera projection matrix
	vec3 lightDirection; ///< Light direction in view space.
	vec3 lightColor; ///< Light intensity.
	float shadowBias; ///< shadow depth bias.
	int shadowMode; ///< The shadow map technique.
	int shadowLayer; ///< The shadow map layer.
};

layout(location = 0) out vec3 fragColor; ///< Color.


/** Compute the lighting contribution of a directional light using the GGX BRDF. */
void main(){
	vec2 uv = In.uv;
	vec4 albedoInfo = textureLod(sampler2D(albedoTexture, sClampNear),uv, 0.0);
	// If emissive (skybox or object), don't shade.
	if(albedoInfo.a == 0.0){
		discard;
	}
	
	// Get all informations from textures.
	vec3 baseColor = albedoInfo.rgb;
	float depth = textureLod(sampler2D(depthTexture, sClampNear),uv, 0.0).r;
	vec3 position = positionFromDepth(depth, uv, projectionMatrix);
	vec3 infos = textureLod(sampler2D(effectsTexture, sClampNear),uv, 0.0).rgb;
	float roughness = max(0.045, infos.r);
	float metallic = infos.g;
	
	vec3 n = normalize(2.0 * textureLod(sampler2D(normalTexture, sClampNear),uv, 0.0).rgb - 1.0);
	vec3 v = normalize(-position);
	vec3 l = normalize(-lightDirection);
	

	// Orientation: basic diffuse shadowing.
	float orientation = max(0.0, dot(l,n));
	// Shadowing
	float shadowing = 1.0;
	if(shadowMode != SHADOW_NONE){
		vec3 lightSpacePosition = (viewToLight * vec4(position,1.0)).xyz;
		lightSpacePosition.xy = 0.5 * lightSpacePosition.xy + 0.5;
		shadowing = shadow(shadowMode, lightSpacePosition, shadowMap, shadowLayer, shadowBias);
	}
	// BRDF contributions.
	// Compute F0 (fresnel coeff).
	// Dielectrics have a constant low coeff, metals use the baseColor (ie reflections are tinted).
	vec3 F0 = mix(vec3(0.04), baseColor, metallic);
	
	// Normalized diffuse contribution. Metallic materials have no diffuse contribution.
	vec3 diffuse = INV_M_PI * (1.0 - metallic) * baseColor * (1.0 - F0);
	
	vec3 specular = ggx(n, v, l, F0, roughness);
	
	fragColor.rgb = shadowing * orientation * (diffuse + specular) * lightColor;
}

