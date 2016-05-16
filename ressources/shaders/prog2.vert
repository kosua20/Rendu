#version 330

// First attribute: vertex position
layout(location = 0) in vec3 v;
// Second attribute: normal
layout(location = 1) in vec3 n;

// Uniform: the MVP matrix
uniform mat4 mvp;

// Output: normal (for interpolation)
out vec3 normal; 

void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);

	normal = n;
}
