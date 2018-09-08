#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

#define INV_M_PI 0.3183098862
#define M_PI 3.1415926536
#define M_INV_LOG2 1.4426950408889

layout(binding = 0) uniform sampler2D albedoTexture; ///< The albedo texture.
layout(binding = 1) uniform sampler2D normalTexture; ///< The normal texture.
layout(binding = 2) uniform sampler2D depthTexture; ///< The depth texture.
layout(binding = 3) uniform sampler2D effectsTexture; ///< The effects texture.
layout(binding = 4) uniform sampler2D ssaoTexture; ///< The SSAO texture.
layout(binding = 5) uniform samplerCube textureCubeMap; ///< Background environment cubemap (with preconvoluted versions of increasing roughness in mipmap levels).
layout(binding = 6) uniform sampler2D brdfPrecalc; ///< Preintegrated BRDF lookup table.

uniform vec3 shCoeffs[9]; ///< SH approximation of the environment irradiance.
uniform mat4 inverseV; ///< The view to world transformation matrix.
uniform vec4 projectionMatrix; ///< The camera projection matrix.

layout(location = 0) out vec3 fragColor; ///< Color.

#define SAMPLES_COUNT 16u
#define MAX_LOD 5

/** Estimate the position of the current fragment in view space based on its depth and camera parameters.
\param depth the depth of the fragment
\return the view space position
*/
vec3 positionFromDepth(float depth){
	float depth2 = 2.0 * depth - 1.0 ;
	vec2 ndcPos = 2.0 * In.uv - 1.0;
	// Linearize depth -> in view space.
	float viewDepth = - projectionMatrix.w / (depth2 + projectionMatrix.z);
	// Compute the x and y components in view space.
	return vec3(- ndcPos * viewDepth / projectionMatrix.xy , viewDepth);
}

/** Evaluate the GGX BRDF for a given normal, view direction and 
	material parameters using a preintegrated BRDF (linear approximation).
	\param n the surface normal
	\param v the view direction
	\param F0 the Fresnel coefficient
	\param roughness the surface roughness
	return the BRDF value
	*/
vec3 ggx(vec3 n, vec3 v, vec3 F0, float roughness){
	// Compute local frame.
	float NdotV = max(0.0, dot(v, n));
	vec3 r = -reflect(v,n);
	r = normalize((inverseV * vec4(r, 0.0)).xyz);
	vec2 brdfParams = texture(brdfPrecalc, vec2(NdotV, roughness)).rg;
	vec3 specularColor = textureLod(textureCubeMap, r, MAX_LOD * roughness).rgb;
	return specularColor * (brdfParams.x * F0 + brdfParams.y);
}

/** Evaluate the ambient irradiance (as SH coefficients) in a given direction. 
	\param wn the direction (normalized)
	\return the ambient irradiance
	*/
vec3 applySH(vec3 wn){
	return (shCoeffs[7] * wn.z + shCoeffs[4]  * wn.y + shCoeffs[8]  * wn.x + shCoeffs[3]) * wn.x +
		   (shCoeffs[5] * wn.z - shCoeffs[8]  * wn.y + shCoeffs[1]) * wn.y +
		   (shCoeffs[6] * wn.z + shCoeffs[2]) * wn.z +
		    shCoeffs[0];
}

/** Compute the environment ambient lighting contribution on the scene. */
void main(){
	
	vec4 albedoInfo = texture(albedoTexture,In.uv);
	
	if(albedoInfo.a == 0){
		// If background (skybox), use directly the diffuse color.
		fragColor = albedoInfo.rgb;
		return;
	}
	
	vec3 baseColor = albedoInfo.rgb;
	vec3 infos = texture(effectsTexture,In.uv).rgb;
	float roughness = max(0.045, infos.r);
	float metallic = infos.g;
	float depth = texture(depthTexture,In.uv).r;
	vec3 position = positionFromDepth(depth);
	vec3 n = normalize(2.0 * texture(normalTexture,In.uv).rgb - 1.0);
	vec3 v = normalize(-position);
	
	// Compute AO.
	float precomputedAO = infos.b;
	float realtimeAO = texture(ssaoTexture, In.uv).r;
	float ao = realtimeAO*precomputedAO;
	
	// Sample illumination envmap using world space normal and SH pre-computed coefficients.
	vec3 worldNormal = normalize(vec3(inverseV * vec4(n,0.0)));
	vec3 envLighting = applySH(worldNormal);
	
	// BRDF contributions.
	// Compute F0 (fresnel coeff).
	// Dielectrics have a constant low coeff, metals use the baseColor (ie reflections are tinted).
	vec3 F0 = mix(vec3(0.08), baseColor, metallic);
	
	// Ambient diffuse contribution. Metallic materials have no diffuse contribution.
	vec3 diffuse = (1.0 - metallic) * baseColor * (1.0 - F0) * envLighting;
	
	// Secular contribution (preintegrated).
	vec3 specular = ggx(n, v, F0, roughness);

	fragColor = ao * (diffuse + specular);
}
