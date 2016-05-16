#version 330

// Input: normal coming from the vertex shader
in vec3 normal; 

// Output: the fragment color
out vec3 fragColor;

void main(){
	// The output color is, for now, the normal scale in [0,1].
	// The interpolation between the vertex and fragment shader does not guarantee the normalization of the normal.
	fragColor = normalize(normal) * 0.5 + 0.5;
}
