#version 330

// Attributes
layout(location = 0) in vec3 v;
layout(location = 1) in vec3 n;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tang;
layout(location = 4) in vec3 binor;

// Uniform: the light structure (position in view space)
layout (std140) uniform Light {
	vec4 position;
	vec4 Ia;
	vec4 Id;
	vec4 Is;
	float shininess;
} light;

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
	vec3 tangentSpacePosition;
	vec3 tangentSpaceView;
	vec3 tangentSpaceLight;
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
	
	Out.tangentSpacePosition = transpose(Out.tbn) * Out.position;
	
	Out.tangentSpaceView = transpose(Out.tbn) * vec3(0.0);
	
	Out.tangentSpaceLight = transpose(Out.tbn) * light.position.xyz;
	
}
