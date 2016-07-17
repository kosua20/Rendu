#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms: the texture, inverse of the screen size, FXAA flag.
uniform sampler2D screenTexture;
uniform vec2 inverseScreenSize;
uniform bool useFXAA;

// Settings for FXAA.
#define EDGE_THRESHOLD_MIN 0.0312
#define EDGE_THRESHOLD_MAX 0.125

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
	
	// Luma at the four direct neighbours of the current fragment.
	float lumaDown = rgb2luma(textureOffset(screenTexture,In.uv,ivec2(0,-1)).rgb);
	float lumaUp = rgb2luma(textureOffset(screenTexture,In.uv,ivec2(0,1)).rgb);
	float lumaLeft = rgb2luma(textureOffset(screenTexture,In.uv,ivec2(-1,0)).rgb);
	float lumaRight = rgb2luma(textureOffset(screenTexture,In.uv,ivec2(1,0)).rgb);
	
	// Find the maximum and minimum luma around the current fragment.
	float lumaMin = min(lumaCenter,min(min(lumaDown,lumaUp),min(lumaLeft,lumaRight)));
	float lumaMax = max(lumaCenter,max(max(lumaDown,lumaUp),max(lumaLeft,lumaRight)));
	
	// Compute the delta.
	float lumaRange = lumaMax - lumaMin;
	
	// If the luma variation is lower that a threshold (or if we are in a really dark area), we are not on an edge, don't perform any AA.
	if(lumaRange < max(EDGE_THRESHOLD_MIN,lumaMax*EDGE_THRESHOLD_MAX)){
		fragColor = colorCenter;
		return;
	}
	
	fragColor = vec3(1.0,0.0,0.0);
	
}
