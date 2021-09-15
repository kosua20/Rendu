#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 1, binding = 0) uniform texture2D screenTexture; ///< Image to blur.

layout(location = 0) out vec3 fragColor; ///< Blurred color.

/** Downscaling step of the dual filtering. */
void main(){

	// Four reads at the corners of the new pixel.
	vec2 texSize = vec2(textureSize(screenTexture, 0).xy);
	vec2 shift = 1.0/texSize;
	vec2 sshift = vec2(-shift.x, shift.y);

	// Color fetches following the described pattern.
	vec3 col = textureLod(sampler2D(screenTexture, sClampLinear), In.uv, 0.0).rgb * 0.5;
	col += textureLod(sampler2D(screenTexture, sClampLinear), In.uv - shift, 0.0).rgb * 0.125;
	col += textureLod(sampler2D(screenTexture, sClampLinear), In.uv + shift, 0.0).rgb * 0.125;
	col += textureLod(sampler2D(screenTexture, sClampLinear), In.uv - sshift, 0.0).rgb * 0.125;
	col += textureLod(sampler2D(screenTexture, sClampLinear), In.uv + sshift, 0.0).rgb * 0.125;
	fragColor = col;
}
