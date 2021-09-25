#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D screenTexture; ///< Image to output.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Just pass the input image as-is, potentially performing up/down scaling. */
void main(){
	
	fragColor.rgb = textureLod(sampler2D(screenTexture, sClampLinear), In.uv, 0.0).rgb;
	fragColor.a = 1.0f;
}
