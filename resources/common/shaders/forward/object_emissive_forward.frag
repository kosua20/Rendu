#version 400

#include "common_pbr.glsl"
#include "common_lights.glsl"

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(binding = 0) uniform sampler2D emissiveTexture; ///< Emissive.

// Emissive objects don't receive any light.

layout (location = 0) out vec4 fragAmbient; ///< Ambient contribution.
layout (location = 1) out vec3 fragDirect; ///< Direct lights contribution.
layout (location = 2) out vec3 fragNormal; ///< Surface normal.

/** Shade the object, applying lighting. */
void main(){
	
	vec4 emissiveColor = texture(emissiveTexture, In.uv);
	if(emissiveColor.a <= 0.01){
		discard;
	}
	fragAmbient = vec4(emissiveColor.rgb, -1.0);
	fragDirect = vec3(0.0);
	fragNormal = vec3(0.5);
}
