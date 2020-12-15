
#include "common_pbr.glsl"
#include "shadow_maps.glsl"
#include "utils.glsl"

// Uniforms
layout(binding = 0) uniform sampler2D albedoTexture; ///< Albedo.
layout(binding = 1) uniform sampler2D normalTexture; ///< Normal.
layout(binding = 2) uniform sampler2D depthTexture; ///< Depth.
layout(binding = 3) uniform sampler2D effectsTexture; ///< Effects.
layout(binding = 4) uniform sampler2DArray shadowMap; ///< Shadow map.

uniform vec4 projectionMatrix; ///< Camera projection matrix
uniform mat4 viewToLight; ///< View to light space matrix.

uniform vec3 lightPosition; ///< Light position in view space.
uniform vec3 lightDirection; ///< Light direction in view space.
uniform vec3 lightColor; ///< Light intensity.
uniform vec2 intOutAnglesCos; ///< Angular attenuation inner and outer angles.
uniform float lightRadius; ///< Attenuation radius.
uniform float shadowBias; ///< shadow depth bias.
uniform int shadowMode; ///< The shadow map technique.
uniform int shadowLayer; ///< The shadow map layer.

layout(location = 0) out vec3 fragColor; ///< Color.


/** Compute the lighting contribution of a spot light using the GGX BRDF. */
void main(){
	
	vec2 uv = gl_FragCoord.xy/textureSize(albedoTexture, 0).xy;
	
	vec4 albedoInfo = textureLod(albedoTexture,uv, 0.0);
	// If emissive (skybox or object), don't shade.
	if(albedoInfo.a == 0.0){
		discard;
	}
	
	// Get all informations from textures.
	vec3 baseColor = albedoInfo.rgb;
	float depth = textureLod(depthTexture,uv, 0.0).r;
	vec3 position = positionFromDepth(depth, uv, projectionMatrix);
	
	vec3 n = normalize(2.0 * textureLod(normalTexture,uv, 0.0).rgb - 1.0);
	vec3 v = normalize(-position);
	vec3 deltaPosition = lightPosition - position;
	vec3 l = normalize(deltaPosition);
	
	// Early exit if we are outside the sphere of influence.
	if(length(deltaPosition) > lightRadius){
		discard;
	}
	// Compute the angle between the light direction and the (light, surface point) vector.
	float currentAngleCos = dot(-l, normalize(lightDirection));
	// If we are outside the spotlight cone, no lighting.
	if(currentAngleCos < intOutAnglesCos.y){
		discard;
	}
	// Compute the spotlight attenuation factor based on our angle compared to the inner and outer spotlight angles.
	float angleAttenuation = clamp((currentAngleCos - intOutAnglesCos.y)/(intOutAnglesCos.x - intOutAnglesCos.y), 0.0, 1.0);
	
	// Orientation: basic diffuse shadowing.
	float orientation = max(0.0, dot(l,n));
	// Shadowing
	float shadowing = 1.0;
	if(shadowMode != SHADOW_NONE){
		vec4 lightSpacePosition = viewToLight * vec4(position,1.0);
		lightSpacePosition /= lightSpacePosition.w;
		shadowing = shadow(shadowMode, 0.5*lightSpacePosition.xyz+0.5, shadowMap, shadowLayer, shadowBias);
	}
	// Attenuation with increasing distance to the light.
	float localRadius2 = dot(deltaPosition, deltaPosition);
	float radiusRatio2 = localRadius2/(lightRadius*lightRadius);
	float attenNum = clamp(1.0 - radiusRatio2, 0.0, 1.0);
	float attenuation = angleAttenuation * attenNum * attenNum;
	
	// Read effects infos.
	vec3 infos = texture(effectsTexture,uv).rgb;
	float roughness = max(0.0001, infos.r);
	float metallic = infos.g;
	
	// BRDF contributions.
	// Compute F0 (fresnel coeff).
	// Dielectrics have a constant low coeff, metals use the baseColor (ie reflections are tinted).
	vec3 F0 = mix(vec3(0.04), baseColor, metallic);
	
	// Normalized diffuse contribution. Metallic materials have no diffuse contribution.
	vec3 diffuse = INV_M_PI * (1.0 - metallic) * baseColor * (1.0 - F0);
	
	vec3 specular = ggx(n, v, l, F0, roughness);
	
	fragColor.rgb = shadowing * attenuation * orientation * (diffuse + specular) * lightColor;
	
}

