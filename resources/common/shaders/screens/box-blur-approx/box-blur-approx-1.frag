#version 330

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to blur.

layout(location = 0) out float fragColor; ///< Color.

/** Perform an approximate 5x5 box blur on the input image, using only 13 samples in a checkerboard pattern. */
void main(){
	
	// We have to unroll the box blur loop manually.
	
	float color;
	color  = textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-2,-2)).r;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-2, 0)).r;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-2, 2)).r;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-1,-1)).r;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-1, 1)).r;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0,-2)).r;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0, 0)).r;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0, 2)).r;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 1,-1)).r;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 1, 1)).r;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 2,-2)).r;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 2, 0)).r;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 2, 2)).r;
	
	fragColor = color / 13.0;
}
