
layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In;

layout(set = 1, binding = 0) uniform sampler2D screenTexture; ///< Image to blur.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Perform aan approximate 5x5 box blur using only 13 samples in a checkerboard pattern. */
void main(){
	
	// We have to unroll the box blur loop manually.
	vec4 color;
	color  = textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-2,-2));
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-2, 0));
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-2, 2));
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-1,-1));
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-1, 1));
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0,-2));
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0, 0));
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0, 2));
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 1,-1));
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 1, 1));
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 2,-2));
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 2, 0));
	color += textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 2, 2));

	fragColor = color / 13.0;
}
