#version 330

// Input: color coming from the vertex shader
in vec3 color; 

// Output: the fragment color
out vec3 fragColor;

void main(){
	// The output color is the interpolated color.
	fragColor = color;
}
