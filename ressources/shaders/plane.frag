#version 330

// Input: tangent space matrix, position (view space) and uv coming from the vertex shader
in INTERFACE {
	vec3 position;
	vec3 normal;
} In ;

// Uniform: the light structure (position in view space)
layout (std140) uniform Light {
  vec4 position;
  vec4 Ia;
  vec4 Id;
  vec4 Is;
  float shininess;
} light;




// Output: the fragment color
out vec3 fragColor;

void main(){
	vec3 n = normalize(In.normal);
	
	// Compute the direction from the point to the light
	vec3 d = normalize(light.position.xyz - In.position);

	vec3 ambient = vec3(0.2);
	vec3 diffuseColor = vec3(0.9);
	
	// Compute the diffuse factor
	float diffuse = max(0.0, dot(d,n));

	vec3 v = normalize(-In.position);
	
	// Compute the specular factor
	float specular = 0.0;
	if(diffuse > 0.0){
		vec3 r = reflect(-d,n);
		specular = pow(max(dot(r,v),0.0),light.shininess);
	}
	
	vec3 shading =  ambient * light.Ia.rgb + diffuse * diffuseColor + specular * light.Is.rgb;
	fragColor = shading;
	
}
