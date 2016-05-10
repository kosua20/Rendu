#version 330

// First attribute: vertex position
layout(location = 0) in vec3 v;

// Output: 2D position
out vec2 position; 

void main(){
	// Simply output the same position, in homogeneous coordinates.
	gl_Position = vec4(v, 1.0);

	position = v.xy;
}
