
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
	
	// Skip unlit.
	if(material.id == MATERIAL_UNLIT){
		discard;
	}
	
	// Retrieve view space position.
	float depth = textureLod(sampler2D(depthTexture, sClampNear), uv, 0.0).r;
	vec3 position = positionFromDepth(depth, uv, projectionMatrix);

	// Populate the light infos.
	Light light;
	light.type = POINT;
	light.position = lightPosition;
	light.radius = lightRadius,
	light.shadowMode = shadowMode;
	light.viewToLight = viewToLight;
	light.layer = shadowLayer;
	light.farPlane = lightFarPlane;
	light.bias = shadowBias;
	light.color = lightColor;

	// Shadowing and light direction.
	float shadowing;
	vec3 l;
	if(!applyPointLight(light, position, shadowMap, l, shadowing)){
		// outside the area of effect of the light.
		discard;
	}

	// Evaluate BRDF.
	vec3 v = normalize(-position);
	vec3 diffuse, specular;
	directBrdf(material, material.normal, v, l, diffuse, specular);

	// Combine everything.
	fragColor.rgb = shadowing * (diffuse + specular) * light.color;
	
}

