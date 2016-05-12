#version 330

// First attribute: vertex position
layout(location = 0) in vec3 v;
// Second attribute: uv coordinates
layout(location = 1) in vec2 t;

// Uniform: the MVP matrix
uniform mat4 mvp;

// Output: UV coordinates (for interpolation)
out vec2 uv; 

void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);

	uv = t;
}
