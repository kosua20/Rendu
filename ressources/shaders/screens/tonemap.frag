#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms: the texture, inverse of the screen size, FXAA flag.
uniform sampler2D screenTexture;
uniform vec2 inverseScreenSize;

// Output: the fragment color
out vec3 fragColor;

vec3 reinhard(vec3 hdrColor){
	return hdrColor / (1.0 + hdrColor);
}

vec3 simpleExposure(vec3 hdrColor, float exposure){
	return 1.0 - exp(-hdrColor * exposure);
}

void main(){
	
	vec3 finalColor = texture(screenTexture,In.uv).rgb;
	fragColor = simpleExposure(finalColor, 0.6);
	
	// Test if any component is still > 1, for demo purposes.
	fragColor = any(greaterThan(fragColor, vec3(1.0))) ? vec3(1.0,0.0,0.0) : fragColor;
	
}
