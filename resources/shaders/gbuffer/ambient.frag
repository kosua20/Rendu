#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

// Uniforms: the texture, inverse of the screen size, FXAA flag.
uniform sampler2D albedoTexture;
uniform sampler2D normalTexture;
uniform sampler2D ssaoTexture;
uniform sampler2D effectsTexture;
uniform samplerCube textureCubeMapSmall;

uniform mat4 inverseV;

// Output: the fragment color
out vec3 fragColor;


void main(){
	
	vec4 albedoInfo = texture(albedoTexture,In.uv);
	
	if(albedoInfo.a == 0){
		// If background (skybox), use directly the diffuse color.
		fragColor = albedoInfo.rgb;
		return;
	}
	
	vec3 baseColor = albedoInfo.rgb;
	vec3 infos = texture(effectsTexture,In.uv).rgb;
	float roughness = max(0.0001, infos.r);
	float metallic = infos.g;
	vec3 n = normalize(2.0 * texture(normalTexture,In.uv).rgb - 1.0);
	
	// Compute AO.
	float precomputedAO = infos.b;
	float realtimeAO = texture(ssaoTexture, In.uv).r;
	float ao = min(realtimeAO, precomputedAO);
	
	// Sample illumination envmap using world space normal.
	vec3 worldNormal = normalize(vec3(inverseV * vec4(n,0.0)));
	vec3 envLighting = texture(textureCubeMapSmall, worldNormal).rgb;
	
	// BRDF contributions.
	// Compute F0 (fresnel coeff).
	// Dielectrics have a constant low coeff, metals use the baseColor (ie reflections are tinted).
	vec3 F0 = mix(vec3(0.08), baseColor, metallic);
	
	// Ambient diffuse contribution. Metallic materials have no diffuse contribution.
	vec3 diffuse = (1.0 - metallic) * baseColor * (1.0 - F0) * envLighting;
	
	// Compute world normal and use it to read into the convolved envmap.
	vec3 specular = vec3(0.0);

	fragColor = ao * (diffuse + specular);
}
