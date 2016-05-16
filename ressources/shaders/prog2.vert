#version 330

// First attribute: vertex position
layout(location = 0) in vec3 v;
// Second attribute: normal
layout(location = 1) in vec3 n;

// Uniform: the MVP, MV and normal matrices
uniform mat4 mvp;
uniform mat4 mv;
uniform mat3 normalMatrix;

// Output: normal and position both in eye space
out vec3 normal;
out vec3 position; 


void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	position = (mv * vec4(v,1.0)).xyz;
	normal = normalMatrix * n;
}
