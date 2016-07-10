#version 330

// Attributes
layout(location = 0) in vec3 v;
layout(location = 1) in vec3 n;

// Uniform: the MVP, MV and normal matrices
uniform mat4 mvp;
uniform mat4 mv;
uniform mat3 normalMatrix;

// Output: tangent space matrix, position in view space and uv.
out INTERFACE {
	vec3 position;
	vec3 normal;
} Out ;


void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);

	Out.position = (mv * vec4(v,1.0)).xyz;

	Out.normal = normalize(normalMatrix * n);
	
}
