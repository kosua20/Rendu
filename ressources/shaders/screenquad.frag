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
	
	fragColor = colorCenter;
}
