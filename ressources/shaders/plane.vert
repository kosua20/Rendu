#version 330

// Attributes
layout(location = 0) in vec3 v;
layout(location = 1) in vec3 n;

// Uniform: the MVP, MV and normal matrices
uniform mat4 mvp;
uniform mat4 mv;
uniform mat3 normalMatrix;
uniform mat4 lightMVP;

// Output: position in view space, normal in view space and position in light space.
out INTERFACE {
	vec3 position;
	vec3 normal;
	vec3 lightSpacePosition;
	vec3 modelPosition;
} Out ;


void main(){
	// We multiply the coordinates by the MVP matrix, and ouput the result.
	gl_Position = mvp * vec4(v, 1.0);

	Out.position = (mv * vec4(v,1.0)).xyz;

	Out.normal = normalize(normalMatrix * n);
	
	Out.lightSpacePosition = 0.5*(lightMVP * vec4(v,1.0)).xyz + 0.5;
	
	Out.modelPosition = v;
	
	
}
