#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In;

layout(set = 2, binding = 0) uniform texture2DArray screenTexture; ///< Image to blur.

layout(set = 0, binding = 0) uniform UniformBlock {
	int layer; ///< The array layer to sample.
};

layout(location = 0) out vec4 fragColor; ///< Color.

/** Perform an approximate 5x5 blur using only 13 samples in a checkerboard pattern. */
void main(){
	
	// We have to unroll the box blur loop manually.
	vec4 color;
	color  = textureLodOffset(sampler2DArray(screenTexture, sClampLinear), vec3(In.uv, layer), 0.0, ivec2(-2,-2));
	color += textureLodOffset(sampler2DArray(screenTexture, sClampLinear), vec3(In.uv, layer), 0.0, ivec2(-2, 0));
	color += textureLodOffset(sampler2DArray(screenTexture, sClampLinear), vec3(In.uv, layer), 0.0, ivec2(-2, 2));
	color += textureLodOffset(sampler2DArray(screenTexture, sClampLinear), vec3(In.uv, layer), 0.0, ivec2(-1,-1));
	color += textureLodOffset(sampler2DArray(screenTexture, sClampLinear), vec3(In.uv, layer), 0.0, ivec2(-1, 1));
	color += textureLodOffset(sampler2DArray(screenTexture, sClampLinear), vec3(In.uv, layer), 0.0, ivec2( 0,-2));
	color += textureLodOffset(sampler2DArray(screenTexture, sClampLinear), vec3(In.uv, layer), 0.0, ivec2( 0, 0));
	color += textureLodOffset(sampler2DArray(screenTexture, sClampLinear), vec3(In.uv, layer), 0.0, ivec2( 0, 2));
	color += textureLodOffset(sampler2DArray(screenTexture, sClampLinear), vec3(In.uv, layer), 0.0, ivec2( 1,-1));
	color += textureLodOffset(sampler2DArray(screenTexture, sClampLinear), vec3(In.uv, layer), 0.0, ivec2( 1, 1));
	color += textureLodOffset(sampler2DArray(screenTexture, sClampLinear), vec3(In.uv, layer), 0.0, ivec2( 2,-2));
	color += textureLodOffset(sampler2DArray(screenTexture, sClampLinear), vec3(In.uv, layer), 0.0, ivec2( 2, 0));
	color += textureLodOffset(sampler2DArray(screenTexture, sClampLinear), vec3(In.uv, layer), 0.0, ivec2( 2, 2));

	fragColor = color / 13.0;
}
