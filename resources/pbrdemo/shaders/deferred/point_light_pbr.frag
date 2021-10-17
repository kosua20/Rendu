
#include "common_pbr.glsl"
#include "shadow_maps.glsl"
#include "utils.glsl"

// Uniforms
layout(set = 2, binding = 0) uniform texture2D albedoTexture; ///< Albedo.
layout(set = 2, binding = 1) uniform texture2D normalTexture; ///< Normal.
layout(set = 2, binding = 2) uniform texture2D depthTexture; ///< Depth.
layout(set = 2, binding = 3) uniform texture2D effectsTexture;///< Effects.
layout(set = 2, binding = 4) uniform textureCubeArray shadowMap; ///< Shadow map.

layout(set = 0, binding = 0) uniform UniformBlock {
	mat4 viewToLight; ///< Light direction in view space.
	vec4 projectionMatrix; ///< Camera projection matrix
	vec3 lightPosition; ///< Light position in view space.
	vec3 lightColor; ///< Light intensity.
	float lightRadius; ///< Attenuation radius.
	float lightFarPlane; ///< Light projection far plane.
	float shadowBias; ///< shadow depth bias.
	int shadowMode; ///< The shadow map technique.
	int shadowLayer; ///< The shadow map layer.
};

layout(location = 0) out vec3 fragColor; ///< Color.

/** Compute the lighting contribution of a point light using the GGX BRDF. */
void main(){
	
	vec2 uv = gl_FragCoord.xy/textureSize(albedoTexture, 0).xy;
	Material material = decodeMaterialFromGbuffer(uv, albedoTexture, normalTexture, effectsTexture);

	// If emissive (skybox or object), don't shade.
	if(material.id == MATERIAL_EMISSIVE){
		discard;
	}

	// Retrieve view space position.
	float depth = textureLod(sampler2D(depthTexture, sClampNear),uv, 0.0).r;
	vec3 position = positionFromDepth(depth, uv, projectionMatrix);

	vec3 v = normalize(-position);
	vec3 deltaPosition = lightPosition - position;
	vec3 l = normalize(deltaPosition);
	// Early exit if we are outside the sphere of influence.
	if(length(deltaPosition) > lightRadius){
		discard;
	}
	// Attenuation with increasing distance to the light.
	float localRadius2 = dot(deltaPosition, deltaPosition);
	float radiusRatio2 = localRadius2/(lightRadius*lightRadius);
	float attenNum = clamp(1.0 - radiusRatio2, 0.0, 1.0);
	float attenuation = attenNum*attenNum;
	
	// Compute the light to surface vector in light centered space.
	// We only care about the direction, so we don't need the translation.
	float shadowing = 1.0;
	if(shadowMode != SHADOW_NONE){
		vec3 deltaPositionWorld = -mat3(viewToLight) * deltaPosition;
		shadowing = shadowCube(shadowMode, deltaPositionWorld, shadowMap, shadowLayer, lightFarPlane, shadowBias);
	}

	// Evaluate BRDF.
	vec3 diffuse, specular;
	directBrdf(material, material.normal, v, l, diffuse, specular);

	// Combine everything.
	fragColor.rgb = shadowing * attenuation * (diffuse + specular) * lightColor;
	
}

