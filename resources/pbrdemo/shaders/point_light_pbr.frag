#version 330

#include "common_pbr.glsl"
#include "shadow_maps.glsl"

// Uniforms
layout(binding = 0) uniform sampler2D albedoTexture; ///< Albedo.
layout(binding = 1) uniform sampler2D normalTexture; ///< Normal.
layout(binding = 2) uniform sampler2D depthTexture; ///< Depth.
layout(binding = 3) uniform sampler2D effectsTexture;///< Effects.
layout(binding = 4) uniform samplerCube shadowMap; ///< Shadow map.

uniform vec4 projectionMatrix; ///< Camera projection matrix
uniform mat3 viewToLight; ///< Light direction in view space.

uniform vec3 lightPosition; ///< Light position in view space.
uniform vec3 lightColor; ///< Light intensity.
uniform float lightRadius; ///< Attenuation radius.
uniform float lightFarPlane; ///< Light projection far plane.
uniform float shadowBias; ///< shadow depth bias.
uniform int shadowMode; ///< The shadow map technique.


layout(location = 0) out vec3 fragColor; ///< Color.


/** Compute the lighting contribution of a point light using the GGX BRDF. */
void main(){
	
	vec2 uv = gl_FragCoord.xy/textureSize(albedoTexture, 0).xy;
	
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
	vec3 deltaPosition = lightPosition - position;
	vec3 l = normalize(deltaPosition);
	// Early exit if we are outside the sphere of influence.
	if(length(deltaPosition) > lightRadius){
		discard;
	}
	// Orientation: basic diffuse shadowing.
	float orientation = max(0.0, dot(l,n));
	// Attenuation with increasing distance to the light.
	float localRadius2 = dot(deltaPosition, deltaPosition);
	float radiusRatio2 = localRadius2/(lightRadius*lightRadius);
	float attenNum = clamp(1.0 - radiusRatio2, 0.0, 1.0);
	float attenuation = attenNum*attenNum;
	
	// Compute the light to surface vector in light centered space.
	// We only care about the direction, so we don't need the translation.
	float shadowing = 1.0;
	if(shadowMode != SHADOW_NONE){
		vec3 deltaPositionWorld = -viewToLight*deltaPosition;
		shadowing = shadowCube(shadowMode, deltaPositionWorld, shadowMap, lightFarPlane, shadowBias);
	}
	
	// BRDF contributions.
	// Compute F0 (fresnel coeff).
	// Dielectrics have a constant low coeff, metals use the baseColor (ie reflections are tinted).
	vec3 F0 = mix(vec3(0.04), baseColor, metallic);
	
	// Normalized diffuse contribution. Metallic materials have no diffuse contribution.
	vec3 diffuse = INV_M_PI * (1.0 - metallic) * baseColor * (1.0 - F0);
	
	vec3 specular = ggx(n, v, l, F0, roughness);
	
	fragColor.rgb = shadowing * attenuation * orientation * (diffuse + specular) * lightColor;
	
}

