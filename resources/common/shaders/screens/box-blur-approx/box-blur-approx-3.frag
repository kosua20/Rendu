#version 330

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to blur.

layout(location = 0) out vec3 fragColor; ///< Color.

/** Perform an approximate 5x5 box blur on the input image, using only 13 samples in a checkerboard pattern. */
void main(){
	
	// We have to unroll the box blur loop manually.
	
	vec3 color;
	color  = textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-2,-2)).rgb;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-2, 0)).rgb;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-2, 2)).rgb;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-1,-1)).rgb;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-1, 1)).rgb;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0,-2)).rgb;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0, 0)).rgb;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0, 2)).rgb;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 1,-1)).rgb;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 1, 1)).rgb;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 2,-2)).rgb;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 2, 0)).rgb;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 2, 2)).rgb;
	
	fragColor = color / 13.0;
}
