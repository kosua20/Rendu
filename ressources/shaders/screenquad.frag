#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniform: the texture.
uniform sampler2D screenTexture;

// Output: the fragment color
out vec3 fragColor;


void main(){
	
	fragColor = texture(screenTexture,In.uv).rgb;
	
}
