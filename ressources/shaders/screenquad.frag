#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniform: the texture.
uniform sampler2D screenTexture;
uniform float time;

// Output: the fragment color
out vec3 fragColor;

// Return a random float in [0,1)
float random(vec2 p){
	return fract(sin(dot(p.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec2 shift(float amplitude){
	
	// Horizontal: high frequency shift to blur locally, depending on the vertical position of the pixel. (noisy horizontal scanlines)
	float horizontalShift = 0.005 * amplitude* fract(time + random(vec2(time,In.uv.y)));
	
	// Vertical: from time to time, shift vertically by a random value.
	float verticalShift = 0.0;
	if(abs(fract(10.0*time)-random(vec2(time,0.0))) < 0.015){
		verticalShift =  0.07 * random(vec2(0.0,time));
	}
	
	return vec2(horizontalShift,verticalShift);
}

void main(){
	
	// Generate a varying amplitude (sin wave depending on the time, with a randomized shift)
	float amplitude = sin(fract(0.1*(time + random(vec2(-time,0.0))))*3.14);
	
	// Generate shifted UVs (for jumpy image effect)
	vec2 shiftedUV = In.uv + shift(amplitude);
	
	// Query the texture, with an additional padding specific for each channel (color abberation simulation).
	float red = textureOffset(screenTexture,shiftedUV,ivec2(-4,0)).r;
	float green = textureOffset(screenTexture,shiftedUV,ivec2(3,1)).g;
	float blue = texture(screenTexture,shiftedUV).b;
	
	// Amplify the green (hue shift)
	green *= 1.1;
	
	// Set the fragment color, modulate with an amplitude (for darkening the image randomly)
	fragColor = (0.6+0.4*amplitude)*vec3(red, green, blue);
	
	// If the y coordinate is in a certain periodic range varying with time, lighten the fragment color (moving scanlines)
	if(fract(gl_FragCoord.y/15.0 + time) < 0.05){
		fragColor += vec3(0.1,0.2,0.15);
	}
	
}
