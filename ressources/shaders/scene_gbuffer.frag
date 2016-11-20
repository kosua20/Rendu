#version 330

// Input: UV coordinates
in INTERFACE {
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


// Uniforms
uniform sampler2D albedoTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;
uniform sampler2D effectsTexture;
uniform samplerCube textureCubeMap;
uniform samplerCube textureCubeMapSmall;
uniform sampler2D shadowMap;

uniform vec2 inverseScreenSize;
uniform vec4 projectionMatrix;
uniform mat4 inverseV;
uniform mat4 lightVP;

// Output: the fragment color
out vec3 fragColor;

vec3 positionFromDepth(float depth){
	float depth2 = 2.0 * depth - 1.0 ;
	vec2 ndcPos = 2.0 * In.uv - 1.0;
	// Linearize depth -> in view space.
	float viewDepth = - projectionMatrix.w / (depth2 + projectionMatrix.z);
	// Compute the x and y components in view space.
	return vec3(- ndcPos * viewDepth / projectionMatrix.xy , viewDepth);
}


void main(){
	
	vec4 albedo =  texture(albedoTexture,In.uv);
	// If this is the skybox, simply output the color.
	if(albedo.a == 0.0){
		fragColor = albedo.rgb;
		return;
	}
	
	vec3 diffuseColor = albedo.rgb;
	
	vec3 n = 2.0*texture(normalTexture,In.uv).rgb - 1.0;
	
	float depth = texture(depthTexture,In.uv).r;
	
	vec3 effects = texture(effectsTexture,In.uv).rgb;
	// If this is the plane, the effects texture contains the depth, manually define values.
	if(albedo.a == 2.0/255.0){
		effects = vec3(1.0,1.0,0.25*(1.0-effects.r));
	}
	
	fragColor = albedo.rgb;
	
}
