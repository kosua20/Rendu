#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D screenTexture; ///< The input image.

layout(location = 0) out vec3 fragColor; ///< Final color.

/** Apply a sharpening filter to the image (based on Uncharted 2 presentation). */
void main(){

	vec3 finalColor = textureLod(sampler2D(screenTexture, sClampLinear), In.uv, 0.0).rgb;
	vec3 down 	= textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 0,-1)).rgb;
	vec3 up 	= textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 0, 1)).rgb;
	vec3 left 	= textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2(-1, 0)).rgb;
	vec3 right 	= textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 1, 0)).rgb;

	fragColor = clamp(finalColor + 0.2*(4.0 * finalColor - down - up - left - right),0.0,1.0);
	
}
