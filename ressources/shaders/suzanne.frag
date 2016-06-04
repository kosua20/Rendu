#version 330

// Input: tangent space matrix, position (view space) and uv coming from the vertex shader
in INTERFACE {
    mat3 tbn;
	vec3 position; 
	vec2 uv;
} In ;

// Uniform: the light structure (position in view space)
layout (std140) uniform Light {
  vec4 position;
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

uniform sampler2D textureColor;
uniform sampler2D textureNormal;
uniform sampler2D textureEffects;
uniform samplerCube textureCubeMap;

uniform mat4 inverseV;

// Output: the fragment color
out vec3 fragColor;

void main(){
	// Compute the normal at the fragment using the tangent space matrix and the normal read in the normal map.
	vec3 n = texture(textureNormal,In.uv).rgb;
	n = normalize(n * 2.0 - 1.0);
	n = normalize(In.tbn * n);

	// Read the effects values
	vec3 effects = texture(textureEffects,In.uv).rgb;

	// Compute the direction from the point to the light
	vec3 d = normalize(light.position.xyz - In.position);

	vec3 diffuseColor = texture(textureColor, In.uv).rgb;

	// The ambient factor
	vec3 ambient = effects.r * diffuseColor * 0.3;
	
	// Compute the diffuse factor
	float diffuse = max(0.0, dot(d,n));

	vec3 v = normalize(-In.position);

	// Compute the specular factor
	float specular = 0.0;
	if(diffuse > 0.0){
		vec3 r = reflect(-d,n);
		specular = pow(max(dot(r,v),0.0),light.shininess);
		specular *= effects.g;
	}
	
	vec3 reflectionColor = vec3(0.0);
	if(effects.b > 0.0){
		vec3 rCubeMap = reflect(-v, n);
		rCubeMap = vec3(inverseV * vec4(rCubeMap,0.0));
		reflectionColor = texture(textureCubeMap,rCubeMap).rgb;
	}

	vec3 shading =  ambient * light.Ia.rgb + diffuse * light.Id.rgb * diffuseColor + specular * light.Is.rgb * material.Ks.rgb ;
	fragColor = mix(shading,reflectionColor,0.5*effects.b);

}
