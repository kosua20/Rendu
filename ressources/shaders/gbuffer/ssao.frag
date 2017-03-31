#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms.
uniform sampler2D depthTexture;
uniform sampler2D normalTexture;

uniform vec2 inverseScreenSize;
uniform mat4 projectionMatrix;

uniform sampler2D noiseTexture; // 5x5 3-components texture with float precision.
uniform vec3 samples[16];

// Output: the fragment color
out float fragColor;


void main(){
	
	vec3 n = normalize(2.0 * texture(normalTexture,In.uv).rgb - 1.0);
	
	// If normal is null, this is the background, no AO.
	if(length(n) < 0.1){
		fragColor = 1.0;
		return;
	}
	
	fragColor = 0.0;
}
