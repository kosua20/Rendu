#version 330

// Input: normal, position coming from the vertex shader
in vec3 normal; 
in vec3 position; 

// Uniform: the light position in view space
uniform vec4 light;

// Output: the fragment color
out vec3 fragColor;

void main(){
	vec3 n = normalize(normal);
	// Compute the direction from the point to the light
	vec3 d = normalize(light.xyz - position);

	// Compute the diffuse factor
	float diffuse = max(0.0, dot(d,n));

	fragColor = vec3(diffuse);

}
