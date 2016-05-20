#version 330

// Input: normal, position coming from the vertex shader
in vec3 normal; 
in vec3 position; 

// Uniform: the light structure (position in view space)
layout (std140) uniform Light {
  vec4 viewPosition;
  vec4 Ia;
  vec4 Id;
  vec4 Is;
  float shininess;
} light;

layout (std140) uniform Material {
  vec4 Ka;
  vec4 Kd;
  vec4 Ks;
} material;

// Output: the fragment color
out vec3 fragColor;

void main(){
	vec3 n = normalize(normal);
	// Compute the direction from the point to the light
	vec3 d = normalize(light.viewPosition.xyz - position);

	// The ambient factor
	float ambient = 0.1;
	
	// Compute the diffuse factor
	float diffuse = max(0.0, dot(d,n));

	// Compute the specular factor
	float specular = 0.0;
	if(diffuse > 0.0){
		vec3 v = normalize(-position);
		vec3 r = reflect(-d,n);
		specular = pow(max(dot(r,v),0.0),light.shininess);
	}

	vec3 shading = ambient * light.Ia.rgb * material.Ka.rgb + diffuse * light.Id.rgb * material.Kd.rgb + specular * light.Is.rgb * material.Ks.rgb ;
	fragColor = shading;

}
