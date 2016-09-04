#version 330

// Input: tangent space matrix, position (view space) and uv coming from the vertex shader
in INTERFACE {
    mat3 tbn;
	vec3 position; 
	vec2 uv;
	vec3 lightSpacePosition;
	vec3 modelPosition;
} In ;

// Uniform: the light structure (position in view space)
layout (std140) uniform Light {
  vec4 position;
  vec4 Ia;
  vec4 Id;
  vec4 Is;
  float shininess;
} light;


uniform sampler2D textureColor;
uniform sampler2D textureNormal;
uniform sampler2D textureEffects;
uniform samplerCube textureCubeMap;
uniform samplerCube textureCubeMapSmall;

uniform sampler2D shadowMap;

uniform mat4 inverseV;

// Output: the fragment color
out vec3 fragColor;

// Returns a random float in [0,1] based on the input vec4 seed.
float random(vec4 p){
	return fract(sin(dot(p, vec4(12.9898,78.233,45.164,94.673))) * 43758.5453);
}


void main(){
	// Compute the normal at the fragment using the tangent space matrix and the normal read in the normal map.
	vec3 n = texture(textureNormal,In.uv).rgb;
	n = normalize(n * 2.0 - 1.0);
	n = normalize(In.tbn * n);

	// Read the effects values
	vec3 effects = texture(textureEffects,In.uv).rgb;

	// Compute the direction from the point to the light
	// light.position.w == 0 if the light is directional, 1 else.
	vec3 d = normalize(light.position.xyz - light.position.w * In.position);

	vec3 diffuseColor = texture(textureColor, In.uv).rgb;
	
	vec3 worldNormal = vec3(inverseV * vec4(n,0.0));
	vec3 lightColor = texture(textureCubeMapSmall,normalize(worldNormal)).rgb;
	diffuseColor = mix(diffuseColor, diffuseColor * lightColor, 0.5);
	
	// The ambient factor
	vec3 ambient = effects.r * 0.3 * diffuseColor;
	
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

	vec3 lightShading = diffuse * diffuseColor + specular * light.Is.rgb;
	
	float shadow = 1.0;
	if (In.lightSpacePosition.z < 1.0){
		// Read first and second moment from shadow map.
		vec2 moments = texture(shadowMap, In.lightSpacePosition.xy).rg;
		// Initial probability of light.
		float probability = float(In.lightSpacePosition.z <= moments.x);
		// Compute variance.
		float variance = moments.y - (moments.x * moments.x);
		variance = max(variance, 0.0001);
		// Delta of depth.
		float delta = In.lightSpacePosition.z - moments.x;
		// Use Chebyshev to estimate bound on probability.
		float probabilityMax = variance / (variance + delta*delta);
		shadow = max(probability, probabilityMax);
		// Limit light bleeding by rescaling and clamping the probability factor.
		shadow = clamp( (shadow - 0.1) / (1.0 - 0.1), 0.0, 1.0);
	}
	
	// Mix the ambient color (always present) with the light contribution, weighted by the shadow factor.
	fragColor = ambient * light.Ia.rgb + shadow * lightShading;
	// Mix with the reflexion color.
	fragColor = mix(fragColor,reflectionColor,0.5*effects.b);
	
}
