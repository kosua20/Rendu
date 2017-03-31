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

float linearizeDepth(float depth){
	float depth2 = 2.0*depth-1.0; // Move from [0,1] to [-1,1].
	float viewDepth = - projectionMatrix[3][2] / (depth2 + projectionMatrix[2][2] );
	return viewDepth;
}

vec3 positionFromUV(vec2 uv){
	// Linearize depth -> in view space.
	float depth = texture(depthTexture, uv).r;
	float viewDepth = linearizeDepth(depth);
	// Compute the x and y components in view space.
	vec2 ndcPos = 2.0 * uv - 1.0;
	return vec3(- ndcPos * viewDepth / vec2(projectionMatrix[0][0], projectionMatrix[1][1] ) , viewDepth);
}

void main(){
	
	vec3 n = normalize(2.0 * texture(normalTexture,In.uv).rgb - 1.0);
	
	// If normal is null, this is the background, no AO.
	if(length(n) < 0.1){
		fragColor = 1.0;
		return;
	}
	
	fragColor = 0.0;
}
