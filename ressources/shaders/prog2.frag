#version 330

// Input: normal, position coming from the vertex shader
flat in vec3 normal; 
in vec3 position; 

// Uniform: the light position in view space
uniform vec4 light;

// Output: the fragment color
out vec3 fragColor;

void main(){
	vec3 n = normalize(normal);
	// Compute the direction from the point to the light
	vec3 d = normalize(light.xyz - position);

	// The ambient factor
	float ambient = 0.1;
	
	// Compute the diffuse factor
	float diffuse = max(0.0, dot(d,n));

	// Compute the specular factor
	float specular = 0.0;
	if(diffuse > 0.0){
		vec3 v = normalize(-position);
		vec3 r = reflect(-d,n);
		specular = pow(max(dot(r,v),0.0),64);
	}

	float shading = ambient + diffuse;// + specular;
	fragColor = vec3(shading);

}
