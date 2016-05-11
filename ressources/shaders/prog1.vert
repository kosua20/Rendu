#version 330

// First attribute: vertex position
layout(location = 0) in vec3 v;
// Second attribute: uv coordinates
layout(location = 1) in vec2 t;

// Output: UV coordinates (for interpolation)
out vec2 uv; 

void main(){
	// Simply output the same position, in homogeneous coordinates.
	gl_Position = vec4(v, 1.0);

	uv = t;
}
