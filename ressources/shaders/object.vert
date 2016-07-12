#version 330

// Attributes
layout(location = 0) in vec3 v;
layout(location = 1) in vec3 n;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tang;
layout(location = 4) in vec3 binor;

// Uniform: the MVP, MV and normal matrices
uniform mat4 mvp;
uniform mat4 mv;
uniform mat3 normalMatrix;
uniform mat4 lightMVP;

// Output: tangent space matrix, position in view space and uv.
out INTERFACE {
    mat3 tbn;
	vec3 position; 
	vec2 uv;
	vec3 lightSpacePosition;
	vec3 modelPosition;
} Out ;


void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);

	Out.position = (mv * vec4(v,1.0)).xyz;

	Out.uv = uv;

	// Compute the TBN matrix (from tangent space to view space).
	vec3 T = normalize(normalMatrix * tang);
	vec3 B = normalize(normalMatrix * binor);
	vec3 N = normalize(normalMatrix * n);
	Out.tbn = mat3(T, B, N);
	
	// Compute position in light space
	Out.lightSpacePosition = 0.5*(lightMVP * vec4(v,1.0)).xyz + 0.5;
	
	Out.modelPosition = v;
	
}
