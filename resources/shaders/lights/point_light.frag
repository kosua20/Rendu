#version 330

// Uniforms
uniform sampler2D albedoTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;
uniform sampler2D effectsTexture;

uniform vec2 inverseScreenSize;
uniform vec4 projectionMatrix;

uniform vec3 lightPosition;//(direction in view space)
uniform vec3 lightColor;
uniform float radius;

// Output: the fragment color
out vec3 fragColor;

vec3 positionFromDepth(float depth, vec2 uv){
	float depth2 = 2.0 * depth - 1.0 ;
	vec2 ndcPos = 2.0 * uv - 1.0;
	// Linearize depth -> in view space.
	float viewDepth = - projectionMatrix.w / (depth2 + projectionMatrix.z);
	// Compute the x and y components in view space.
	return vec3(- ndcPos * viewDepth / projectionMatrix.xy , viewDepth);
}


vec3 lambert(){
	return vec3(0.0);
}

vec3 ggx(){
	return vec3(0.0);
}

void main(){
	vec2 uv = gl_FragCoord.xy*inverseScreenSize;
	
	vec4 albedoInfo =  texture(albedoTexture,uv);
	// If this is the skybox, don't shade.
	if(albedoInfo.a == 0.0){
		discard;
	}
	
	// Get all informationsfrom textures.
	vec3 albedo = albedoInfo.rgb;
	float depth = texture(depthTexture,uv).r;
	vec3 position = positionFromDepth(depth, uv);
	vec3 infos = texture(effectsTexture,uv).rgb;
	
	vec3 n = 2.0 * texture(normalTexture,uv).rgb - 1.0;
	vec3 v = normalize(-position);
	vec3 l = normalize(lightPosition - position);
	
	// Orientation: basic diffuse shadowing.
	float orientation = max(0.0, dot(l,n));
	// Attenuation with increasing distance to the light.
	float attenuation = pow(max(0.0, 1.0 - distance(position,lightPosition)/radius),2);
	
	// BRDF contributions.
	vec3 diffuse = lambert();
	vec3 specular = ggx();
	
	fragColor.rgb = attenuation * orientation * (diffuse + specular) * lightColor;
	
}

