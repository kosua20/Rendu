#version 330

// Input: position in model space
in INTERFACE {
	vec3 position; 
} In ;


// Output: the fragment color
out vec3 fragColor;

void main(){
	
	fragColor = In.position * 0.5 + 0.5;

}
