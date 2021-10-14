
#include "common_pbr.glsl"
#include "utils.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D albedoTexture; ///< Albedo texture.
layout(set = 2, binding = 1) uniform texture2D probeAccumulatedTexture; ///< Accumulated probes lighting.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Normalize environment ambient lighting contribution on the scene and apply the background. */
void main(){
	fragColor.a = 1.0;

	// If emissive (skybox or object, ID 0) use directly the albedo color.
	vec4 albedoInfo = texture(sampler2D(albedoTexture, sClampNear), In.uv);
	if(albedoInfo.a == 0.0){
		fragColor.rgb = albedoInfo.rgb;
		return;
	}
	// Normalize ambient lighting.
	vec4 lighting = texture(sampler2D(probeAccumulatedTexture, sClampNear), In.uv);
	fragColor.rgb = lighting.a != 0.0 ? (lighting.rgb / lighting.a) : vec3(0.0);
}
