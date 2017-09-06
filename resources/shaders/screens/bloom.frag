#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms: the texture, inverse of the screen size.
uniform sampler2D screenTexture;

// Output: the fragment color
out vec3 fragColor;


void main(){
	fragColor = vec3(0.0);
	
	vec3 color = texture(screenTexture, In.uv).rgb;
	// Compute intensity (luminance). If > 1.0, bloom should be visible.
	if(dot(color, vec3(0.289, 0.527, 0.184)) > 1.0){
		fragColor = color;
	}
}
