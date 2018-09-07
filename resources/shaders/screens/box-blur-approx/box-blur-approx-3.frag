#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms: the texture, inverse of the screen size.
layout(binding = 0) uniform sampler2D screenTexture;

// Output: the fragment color
layout(location = 0) out vec3 fragColor;


void main(){
	
	// We have to unroll the box blur loop manually.
	
	vec3 color = textureOffset(screenTexture, In.uv, ivec2(-2,-2)).rgb;
	color += textureOffset(screenTexture, In.uv, ivec2(-2,0)).rgb;
	color += textureOffset(screenTexture, In.uv, ivec2(-2,2)).rgb;
	
	color += textureOffset(screenTexture, In.uv, ivec2(-1,-1)).rgb;
	color += textureOffset(screenTexture, In.uv, ivec2(-1,1)).rgb;
	
	color += textureOffset(screenTexture, In.uv, ivec2(0,-2)).rgb;
	color += textureOffset(screenTexture, In.uv, ivec2(0,0)).rgb;
	color += textureOffset(screenTexture, In.uv, ivec2(0,2)).rgb;
	
	color += textureOffset(screenTexture, In.uv, ivec2(1,-1)).rgb;
	color += textureOffset(screenTexture, In.uv, ivec2(1,1)).rgb;
	
	color += textureOffset(screenTexture, In.uv, ivec2(2,-2)).rgb;
	color += textureOffset(screenTexture, In.uv, ivec2(2,0)).rgb;
	color += textureOffset(screenTexture, In.uv, ivec2(2,2)).rgb;
	
	fragColor = color / 13.0;
}
