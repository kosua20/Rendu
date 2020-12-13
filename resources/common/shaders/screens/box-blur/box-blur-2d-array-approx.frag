
in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In;

layout(binding = 0) uniform sampler2DArray screenTexture; ///< Image to blur.
uniform int layer; ///< The array layer to sample.
layout(location = 0) out vec4 fragColor; ///< Color.

/** Perform an approximate 5x5 blur using only 13 samples in a checkerboard pattern. */
void main(){
	
	// We have to unroll the box blur loop manually.
	vec4 color;
	color  = textureLodOffset(screenTexture, vec3(In.uv, layer), 0.0, ivec2(-2,-2));
	color += textureLodOffset(screenTexture, vec3(In.uv, layer), 0.0, ivec2(-2, 0));
	color += textureLodOffset(screenTexture, vec3(In.uv, layer), 0.0, ivec2(-2, 2));
	color += textureLodOffset(screenTexture, vec3(In.uv, layer), 0.0, ivec2(-1,-1));
	color += textureLodOffset(screenTexture, vec3(In.uv, layer), 0.0, ivec2(-1, 1));
	color += textureLodOffset(screenTexture, vec3(In.uv, layer), 0.0, ivec2( 0,-2));
	color += textureLodOffset(screenTexture, vec3(In.uv, layer), 0.0, ivec2( 0, 0));
	color += textureLodOffset(screenTexture, vec3(In.uv, layer), 0.0, ivec2( 0, 2));
	color += textureLodOffset(screenTexture, vec3(In.uv, layer), 0.0, ivec2( 1,-1));
	color += textureLodOffset(screenTexture, vec3(In.uv, layer), 0.0, ivec2( 1, 1));
	color += textureLodOffset(screenTexture, vec3(In.uv, layer), 0.0, ivec2( 2,-2));
	color += textureLodOffset(screenTexture, vec3(In.uv, layer), 0.0, ivec2( 2, 0));
	color += textureLodOffset(screenTexture, vec3(In.uv, layer), 0.0, ivec2( 2, 2));

	fragColor = color / 13.0;
}
