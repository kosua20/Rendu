#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In;

layout(set = 2, binding = 0) uniform texture2D screenTexture; ///< Image to blur.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Perform a 5x5 box blur on the input image using 25 samples. */
void main(){
	
	// We have to unroll the box blur loop manually.
	
	vec4 color;
	color  = textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2(-2,-2));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2(-2, 0));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2(-2, 2));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2(-1,-1));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2(-1, 1));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 0,-2));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 0, 0));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 0, 2));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 1,-1));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 1, 1));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 2,-2));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 2, 0));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 2, 2));
	
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2(-2,-1));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2(-2, 1));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2(-1,-2));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2(-1, 0));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2(-1, 2));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 0,-1));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 0, 1));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 1,-2));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 1, 0));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 1, 2));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 2,-1));
	color += textureLodOffset(sampler2D(screenTexture, sClampLinear), In.uv, 0.0, ivec2( 2, 1));

	fragColor = color / 25.0;
}
