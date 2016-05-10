#version 330

// Input: 2D position coming from the vertex shader
in vec2 position; 

// Output: the fragment color
out vec3 fragColor;

void main(){
	// The output color is based on the position of the fragment.
	// We scale/translate it from [-1,1] to [0,1] 
	vec2 positionScaled = 0.5*position+0.5;
	fragColor = vec3(positionScaled,0.0);
}
