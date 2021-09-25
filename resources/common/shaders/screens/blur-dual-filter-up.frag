#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D screenTexture; ///< Image to blur.

layout(location = 0) out vec3 fragColor; ///< Blurred color.

/** Upscaling step of the dual filtering. */
void main(){

	// Compute pattern shifts.
	vec2 texSize = vec2(textureSize(screenTexture, 0).xy);
	vec2 shift = 0.5/texSize;
	vec2 sshift = vec2(-shift.x, shift.y);
	vec2 shiftX = vec2(2.0*shift.x, 0.0);
	vec2 shiftY = vec2(0.0, 2.0*shift.y);

	// Color fetches following the described pattern.
	vec3 col;
	col  = textureLod(sampler2D(screenTexture, sClampLinear), In.uv - shiftX, 0.0).rgb / 12.0;
	col += textureLod(sampler2D(screenTexture, sClampLinear), In.uv + shiftX, 0.0).rgb / 12.0;
	col += textureLod(sampler2D(screenTexture, sClampLinear), In.uv - shiftY, 0.0).rgb / 12.0;
	col += textureLod(sampler2D(screenTexture, sClampLinear), In.uv + shiftY, 0.0).rgb / 12.0;

	col += textureLod(sampler2D(screenTexture, sClampLinear), In.uv - shift , 0.0).rgb / 6.0;
	col += textureLod(sampler2D(screenTexture, sClampLinear), In.uv + shift , 0.0).rgb / 6.0;
	col += textureLod(sampler2D(screenTexture, sClampLinear), In.uv - sshift, 0.0).rgb / 6.0;
	col += textureLod(sampler2D(screenTexture, sClampLinear), In.uv + sshift, 0.0).rgb / 6.0;

	fragColor = col;
}
