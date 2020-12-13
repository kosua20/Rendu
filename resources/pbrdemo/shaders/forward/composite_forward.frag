
#include "common_pbr.glsl"

in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

layout(binding = 0) uniform sampler2D ambientTexture; ///< The ambient contribution.
layout(binding = 1) uniform sampler2D directTexture; ///< The diffuse contribution.
layout(binding = 2) uniform sampler2D ssaoTexture; ///< The SSAO texture.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Combine lighting contributions, modulating ambient using SSAO. */
void main(){
	fragColor.a = 1.0;

	vec4 ambient = textureLod(ambientTexture, In.uv, 0.0);
	float precomputedAO = ambient.a;
	// Skip background.
	if(precomputedAO < -0.01){
		fragColor.rgb = ambient.rgb;
		return;
	}
	vec3 direct = textureLod(directTexture, In.uv, 0.0).rgb;
	// Compute AO.
	float realtimeAO = textureLod(ssaoTexture, In.uv, 0.0).r;
	// Compared to deferred shading, and because we don't want to have too many screen buffers in use,
	// we fall back to using the same occlusion term for both diffuse and specular ambient.
	float aoDiffuse = min(realtimeAO, precomputedAO);
	float ao = aoDiffuse;
	
	fragColor.rgb = ao * ambient.rgb + direct;
	fragColor.a = 1.0;
}
