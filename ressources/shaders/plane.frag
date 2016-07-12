#version 330

// Input: position in view space, normal in view space and position in light space
in INTERFACE {
	vec3 position;
	vec3 normal;
	vec3 lightSpacePosition;
} In ;

// Uniform: the light structure (position in view space)
layout (std140) uniform Light {
  vec4 position;
  vec4 Ia;
  vec4 Id;
  vec4 Is;
  float shininess;
} light;


uniform sampler2D shadowMap;

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
	
	vec3 lightShading =  diffuse * diffuseColor + specular * light.Is.rgb;
	
	
	// Shadows
	float shadow = 0.0;
	float bias = 0.005;
	if(In.lightSpacePosition.z < 1.0){
		float depthLight = texture(shadowMap,In.lightSpacePosition.xy).r;
		if(In.lightSpacePosition.z - depthLight > bias){
			shadow = 1.0;
		}
	}
	
	fragColor = ambient * light.Ia.rgb + (1.0-shadow)*lightShading;
	
}
