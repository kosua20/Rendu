#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms: the texture, inverse of the screen size, FXAA flag.
uniform sampler2D screenTexture;
uniform vec2 inverseScreenSize;
uniform bool useFXAA;

// Output: the fragment color
out vec3 fragColor;

// Return the luma value in perceptual space for a given RGB color in linear space.
float rgb2luma(vec3 rgb){
	return sqrt(dot(rgb, vec3(0.299, 0.587, 0.114)));
}

void main(){
	
	vec3 colorCenter = texture(screenTexture,In.uv).rgb;
	
	// If AA disabled, return directly the fragment color.
	if(!useFXAA){
		fragColor = colorCenter;
		return;
	}
	
	// Display a small green square in the bottom-left corner if AA is enabled.
	if(max(gl_FragCoord.x, gl_FragCoord.y) < 20){
		fragColor = vec3(0.2,0.8,0.4);
		return;
	}
	
	// Luma at the current fragment
	float lumaCenter = rgb2luma(colorCenter);

	fragColor = vec3(lumaCenter);
	
}
