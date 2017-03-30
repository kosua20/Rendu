#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms: the texture, inverse of the screen size, FXAA flag.
uniform sampler2D depthTexture;
uniform sampler2D normalTexture;
uniform sampler2D noiseTexture;
uniform vec2 inverseScreenSize;
uniform vec4 projectionMatrix;
uniform vec3 samples[16];
// Output: the fragment color
out float fragColor;


void main(){
	vec2 uvs = (gl_FragCoord.xy+0.5)*0.2;
	//fragColor = texture(noiseTexture, uvs).r;
	// 48 elements
	vec2 ind1 = floor(In.uv*vec2(15.9999, 2.999));
	int subInd = int(floor(ind1.y));
	int ind = int(floor(ind1.x));
	fragColor = samples[ind][subInd];
}
