#version 330

// Attributes
layout(location = 0) in vec3 v;

// Uniform: the MVP, MV and normal matrices
uniform mat4 mvp;

void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	
}
