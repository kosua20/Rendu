
#include "common_pbr.glsl"
#include "forward_lights.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 1, binding = 0) uniform sampler2D emissiveTexture; ///< Emissive.

// Emissive objects don't receive any light.

layout (location = 0) out vec4 fragColor; ///< Ambient contribution.

/** Shade the object, applying lighting. */
void main(){
	
	vec4 emissiveColor = texture(emissiveTexture, In.uv);
	if(emissiveColor.a <= 0.01){
		discard;
	}

	fragColor = vec4(emissiveColor.rgb, -1.0);
}
