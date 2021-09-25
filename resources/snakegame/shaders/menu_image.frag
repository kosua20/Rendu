#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D imageTexture; ///< Image to display.
layout(location = 0) out vec4 fragColor; ///< Color.

/** Apply the image. */
void main(){
	fragColor = texture(sampler2D(imageTexture, sClampLinearLinear), In.uv);
}
