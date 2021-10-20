
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
	Material material = decodeMaterialFromGbuffer(uv, albedoTexture, normalTexture, effectsTexture);
	
	// Skip unlit.
	if(material.id == MATERIAL_UNLIT){
		discard;
	}
	
	// Retrieve view space position.
	float depth = textureLod(sampler2D(depthTexture, sClampNear),uv, 0.0).r;
	vec3 position = positionFromDepth(depth, uv, projectionMatrix);

	// Populate light info.
	Light light;
	light.type = DIRECTIONAL;
	light.viewToLight = viewToLight;
	light.direction = lightDirection;
	light.color = lightColor;
	light.shadowMode = shadowMode;
	light.layer = shadowLayer;
	light.bias = shadowBias;

	// Light shadowing and attenuation.
	vec3 l;
	float shadowing;
	if(!applyDirectionalLight(light, position, shadowMap, l, shadowing)){
		// Outside the area of effect.
		discard;
	}

	// Evaluate BRDF.
	vec3 v = normalize(-position);
	vec3 diffuse, specular;
	directBrdf(material, material.normal, v, l, diffuse, specular);

	// Combine everything.
	fragColor.rgb = shadowing * (diffuse + specular) * light.color;
}

