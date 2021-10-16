#include "common_pbr.glsl"
#include "utils.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D albedoTexture; ///< Albedo texture.
layout(set = 2, binding = 1) uniform texture2D effectsTexture; ///< Effects texture.
layout(set = 2, binding = 2) uniform texture2D probeAccumulatedTexture; ///< Accumulated probes lighting.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Normalize environment ambient lighting contribution on the scene and apply the background. */
void main(){
	fragColor.a = 1.0;

	vec4 albedoInfo = textureLod(sampler2D(albedoTexture, sClampNear), In.uv, 0.0);
	// If emissive (skybox or object, ID 0) output directly the emissive color.
	uint material = decodeMaterial(albedoInfo.a);
	if(material == MATERIAL_EMISSIVE){
		// Re-apply emissive intensity.
		vec4 effects = textureLod(sampler2D(effectsTexture, sClampNear), In.uv, 0.0);
		fragColor.rgb = albedoInfo.rgb * vec4ToFloat(effects);
		return;
	}
	// Normalize ambient lighting.
	vec4 lighting = textureLod(sampler2D(probeAccumulatedTexture, sClampNear), In.uv, 0.0);
	fragColor.rgb = lighting.a != 0.0 ? (lighting.rgb / lighting.a) : vec3(0.0);
}
