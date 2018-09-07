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
	
	fragColor = texture(screenTexture, In.uv).rgb;
	
}
