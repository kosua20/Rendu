#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to blur.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Perform an approximate 5x5 box blur on the input image, using only 13 samples in a checkerboard pattern. */
void main(){
	
	// We have to unroll the box blur loop manually.
	
	vec4 color = textureOffset(screenTexture, In.uv, ivec2(-2,-2));
	color += textureOffset(screenTexture, In.uv, ivec2(-2,0));
	color += textureOffset(screenTexture, In.uv, ivec2(-2,2));
	
	color += textureOffset(screenTexture, In.uv, ivec2(-1,-1));
	color += textureOffset(screenTexture, In.uv, ivec2(-1,1));
	
	color += textureOffset(screenTexture, In.uv, ivec2(0,-2));
	color += textureOffset(screenTexture, In.uv, ivec2(0,0));
	color += textureOffset(screenTexture, In.uv, ivec2(0,2));
	
	color += textureOffset(screenTexture, In.uv, ivec2(1,-1));
	color += textureOffset(screenTexture, In.uv, ivec2(1,1));
	
	color += textureOffset(screenTexture, In.uv, ivec2(2,-2));
	color += textureOffset(screenTexture, In.uv, ivec2(2,0));
	color += textureOffset(screenTexture, In.uv, ivec2(2,2));
	
	fragColor = color / 13.0;
}
