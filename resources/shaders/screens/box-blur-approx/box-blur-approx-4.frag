#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms: the texture, inverse of the screen size.
layout(binding = 0) uniform sampler2D screenTexture;

// Output: the fragment color
layout(location = 0) out vec4 fragColor;


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
