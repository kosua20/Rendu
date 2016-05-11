#version 330

// Input: UV coordinates coming from the vertex shader
in vec2 uv; 

// Uniform: time
uniform float time;
// Uniform: texture sampler
uniform sampler2D texture1;

// Output: the fragment color
out vec3 fragColor;

void main(){
	// The output color is read from the texture, suing the UV coordinates.
	fragColor = texture(texture1, uv).rgb;
}
