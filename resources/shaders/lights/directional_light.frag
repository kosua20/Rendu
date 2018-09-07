#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

#define INV_M_PI 0.3183098862
#define M_PI 3.1415926536

// Uniforms
layout(binding = 0) uniform sampler2D albedoTexture;
layout(binding = 1) uniform sampler2D normalTexture;
layout(binding = 2) uniform sampler2D depthTexture;
layout(binding = 3) uniform sampler2D effectsTexture;
layout(binding = 4) uniform sampler2D shadowMap;

uniform vec4 projectionMatrix;
uniform mat4 viewToLight;

uniform vec3 lightDirection;//(direction in view space)
uniform vec3 lightColor;
uniform bool castShadow;

// Output: the fragment color
layout(location = 0) out vec3 fragColor;

vec3 positionFromDepth(float depth){
	float depth2 = 2.0 * depth - 1.0 ;
	vec2 ndcPos = 2.0 * In.uv - 1.0;
	// Linearize depth -> in view space.
	float viewDepth = - projectionMatrix.w / (depth2 + projectionMatrix.z);
	// Compute the x and y components in view space.
	return vec3(- ndcPos * viewDepth / projectionMatrix.xy , viewDepth);
}


// Compute the shadow multiplicator based on shadow map.

float shadow(vec3 lightSpacePosition){
	float probabilityMax = 1.0;
	// Read first and second moment from shadow map.
	vec2 moments = texture(shadowMap, lightSpacePosition.xy).rg;
	if(moments.x >= 1.0){
		// No information in the depthmap: no occluder.
		return 1.0;
	}
	// Initial probability of light.
	float probability = float(lightSpacePosition.z <= moments.x);
	// Compute variance.
	float variance = moments.y - (moments.x * moments.x);
	variance = max(variance, 0.00001);
	// Delta of depth.
	float d = lightSpacePosition.z - moments.x;
	// Use Chebyshev to estimate bound on probability.
	probabilityMax = variance / (variance + d*d);
	probabilityMax = max(probability, probabilityMax);
	// Limit light bleeding by rescaling and clamping the probability factor.
	probabilityMax = clamp( (probabilityMax - 0.1) / (1.0 - 0.1), 0.0, 1.0);
	return probabilityMax;
}

vec3 F(vec3 F0, float VdotH){
	float approx = pow(2.0, (-5.55473 * VdotH - 6.98316) * VdotH);
	return F0 + approx * (1.0 - F0);
}

float D(float NdotH, float alpha){
	float halfDenum = NdotH * NdotH * (alpha * alpha - 1.0) + 1.0;
	float halfTerm = alpha / max(0.0001, halfDenum);
	return halfTerm * halfTerm * INV_M_PI;
}

float G1(float NdotX, float halfAlpha){
	return 1.0 / max(0.0001, (NdotX * (1.0 - halfAlpha) + halfAlpha));
}

float G(float NdotL, float NdotV, float alpha){
	float halfAlpha = alpha * 0.5;
	return G1(NdotL, halfAlpha)*G1(NdotV, halfAlpha);
}

vec3 ggx(vec3 n, vec3 v, vec3 l, vec3 F0, float roughness){
	// Compute half-vector.
	vec3 h = normalize(v+l);
	// Compute all needed dot products.
	float NdotL = clamp(dot(n,l), 0.0, 1.0);
	float NdotV = clamp(dot(n,v), 0.0, 1.0);
	float NdotH = clamp(dot(n,h), 0.0, 1.0);
	float VdotH = clamp(dot(v,h), 0.0, 1.0);
	float alpha = max(0.0001, roughness*roughness);
	
	return D(NdotH, alpha) * G(NdotL, NdotV, alpha) * 0.25 * F(F0, VdotH);
}

void main(){
	vec2 uv = In.uv;
	vec4 albedoInfo = texture(albedoTexture,uv);
	// If this is the skybox, don't shade.
	if(albedoInfo.a == 0.0){
		discard;
	}
	
	// Get all informations from textures.
	vec3 baseColor = albedoInfo.rgb;
	float depth = texture(depthTexture,uv).r;
	vec3 position = positionFromDepth(depth);
	vec3 infos = texture(effectsTexture,uv).rgb;
	float roughness = max(0.045, infos.r);
	float metallic = infos.g;
	
	vec3 n = 2.0 * texture(normalTexture,uv).rgb - 1.0;
	vec3 v = normalize(-position);
	vec3 l = normalize(-lightDirection);
	

	// Orientation: basic diffuse shadowing.
	float orientation = max(0.0, dot(l,n));
	// Shadowing
	float shadowing = 1.0;
	if(castShadow){
		vec3 lightSpacePosition = 0.5*(viewToLight * vec4(position,1.0)).xyz + 0.5;
		shadowing = shadow(lightSpacePosition);
	}
	// BRDF contributions.
	// Compute F0 (fresnel coeff).
	// Dielectrics have a constant low coeff, metals use the baseColor (ie reflections are tinted).
	vec3 F0 = mix(vec3(0.08), baseColor, metallic);
	
	// Normalized diffuse contribution. Metallic materials have no diffuse contribution.
	vec3 diffuse = INV_M_PI * (1.0 - metallic) * baseColor * (1.0 - F0);
	
	vec3 specular = ggx(n, v, l, F0, roughness);
	
	fragColor.rgb = shadowing * orientation * (diffuse + specular) * lightColor * M_PI;
}

