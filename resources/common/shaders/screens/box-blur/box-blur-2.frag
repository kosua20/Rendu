#version 330

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to blur.

layout(location = 0) out vec2 fragColor; ///< Color.

/** Perform a 5x5 box blur on the input image. */
void main(){
	
	// We have to unroll the box blur loop manually.
	
	vec2 color;
	color  = textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-2,-2)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-2,-1)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-2, 0)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-2, 1)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-2, 2)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-1,-2)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-1,-1)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-1, 0)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-1, 1)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-1, 2)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0,-2)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0,-1)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0, 0)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0, 1)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0, 2)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 1,-2)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 1,-1)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 1, 0)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 1, 1)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 1, 2)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 2,-2)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 2,-1)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 2, 0)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 2, 1)).rg;
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 2, 2)).rg;
	
	fragColor = color / 25.0;
}
