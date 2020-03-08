#version 330

#include "common_pbr.glsl"
#include "shadow_maps.glsl"

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

// Uniforms
layout(binding = 0) uniform sampler2D albedoTexture; ///< Albedo.
layout(binding = 1) uniform sampler2D normalTexture; ///< Normal.
layout(binding = 2) uniform sampler2D depthTexture; ///< Depth.
layout(binding = 3) uniform sampler2D effectsTexture; ///< Effects.
layout(binding = 4) uniform sampler2D shadowMap; ///< Shadow map.

uniform vec4 projectionMatrix; ///< Camera projection matrix
uniform mat4 viewToLight; ///< View to light space matrix.

uniform vec3 lightDirection; ///< Light direction in view space.
uniform vec3 lightColor; ///< Light intensity.
uniform bool castShadow; ///< Should the shadow map be used.
uniform float shadowBias; ///< shadow depth bias.
uniform int shadowMode; ///< The shadow map technique.


layout(location = 0) out vec3 fragColor; ///< Color.


/** Compute the lighting contribution of a directional light using the GGX BRDF. */
void main(){
	vec2 uv = In.uv;
	vec4 albedoInfo = textureLod(albedoTexture,uv, 0.0);
	// If this is the skybox, don't shade.
	if(albedoInfo.a == 0.0){
		discard;
	}
	
	// Get all informations from textures.
	vec3 baseColor = albedoInfo.rgb;
	float depth = textureLod(depthTexture,uv, 0.0).r;
	vec3 position = positionFromDepth(depth, uv, projectionMatrix);
	vec3 infos = textureLod(effectsTexture,uv, 0.0).rgb;
	float roughness = max(0.045, infos.r);
	float metallic = infos.g;
	
	vec3 n = 2.0 * textureLod(normalTexture,uv, 0.0).rgb - 1.0;
	vec3 v = normalize(-position);
	vec3 l = normalize(-lightDirection);
	

	// Orientation: basic diffuse shadowing.
	float orientation = max(0.0, dot(l,n));
	// Shadowing
	float shadowing = 1.0;
	if(castShadow){
		vec3 lightSpacePosition = 0.5*(viewToLight * vec4(position,1.0)).xyz + 0.5;
		shadowing = shadow(shadowMode, lightSpacePosition, shadowMap, shadowBias);
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

