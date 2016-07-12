#version 330

// Input: position in view space, normal in view space and position in light space
in INTERFACE {
	vec3 position;
	vec3 normal;
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


uniform sampler2D shadowMap;

// Output: the fragment color
out vec3 fragColor;

// Constant: vectors for Poisson disk sampling.
const vec2 poissonDisk[16] = vec2[](
	vec2(-0.94201624, -0.39906216),
	vec2(0.94558609, -0.76890725),
	vec2(-0.094184101, -0.92938870),
	vec2(0.34495938, 0.29387760),
	vec2(-0.91588581, 0.45771432),
	vec2(-0.81544232, -0.87912464),
	vec2(-0.38277543, 0.27676845),
	vec2(0.97484398, 0.75648379),
	vec2(0.44323325, -0.97511554),
	vec2(0.53742981, -0.47373420),
	vec2(-0.26496911, -0.41893023),
	vec2(0.79197514, 0.19090188),
	vec2(-0.24188840, 0.99706507),
	vec2(-0.81409955, 0.91437590),
	vec2(0.19984126, 0.78641367),
	vec2(0.14383161, -0.14100790)
);

// Returns a random float in [0,1] based on the input vec4 seed.
float random(vec4 p){
	return fract(sin(dot(p, vec4(12.9898,78.233,45.164,94.673))) * 43758.5453);
}

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
	
	// If the depth at the current fragment is too high, we are necessarily out of the light frustum, no shadow. Else:
	if(In.lightSpacePosition.z < 1.0){
		// PCF sampling: draw eight samples at random around the current position in the shadow map.
		for (int i=0;i<8;i++){
			// Draw a random float in [0,1], based on the position of the fragment in world space, scaled and floored to avoid repetitions, and on the current sample index.
			float randomValue = random(vec4(floor(In.modelPosition*7500.0), i));
			// Compute a [0-15] index using this random float.
			int randomIndex = int(16.0*randomValue)%16;
			// Compute the shifted position using the poisson disk reference vectors.
			vec2 shiftedPosition = In.lightSpacePosition.xy + poissonDisk[randomIndex]/500.0;
			// Query the corresponding depth in the shadow map.
			float depthLight = texture(shadowMap, shiftedPosition).r;
			// If the fragment is in the shadow, increment the shadow value.
			shadow += (In.lightSpacePosition.z - depthLight  > bias) ? 0.125 : 0;
		}
	}
	
	// Mix the ambient color (always present) with the light contribution, weighted by the shadow factor.
	fragColor = ambient * light.Ia.rgb + (1.0-shadow)*lightShading;
	
}
