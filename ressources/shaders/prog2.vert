#version 330

// First attribute: vertex position
layout(location = 0) in vec3 v;
// Second attribute: normal
layout(location = 1) in vec3 n;
// Second attribute: UV
layout(location = 2) in vec2 uv;

// Uniform: the MVP, MV and normal matrices
uniform mat4 mvp;
uniform mat4 mv;
uniform mat3 normalMatrix;

// Output: normal and position both in eye space
out INTERFACE {
    vec3 normal;
	vec3 position; 
	vec2 uv;
} Out ;


void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);
	Out.position = (mv * vec4(v,1.0)).xyz;
	Out.normal = normalMatrix * n;
	Out.uv = uv;
}
