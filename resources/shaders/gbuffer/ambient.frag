#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ;

#define INV_M_PI 0.3183098862
#define M_PI 3.1415926536
#define M_INV_LOG2 1.4426950408889

// Uniforms: the texture, inverse of the screen size, FXAA flag.
uniform sampler2D albedoTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;
uniform sampler2D effectsTexture;
uniform sampler2D ssaoTexture;
uniform samplerCube textureCubeMap;
uniform samplerCube textureCubeMapSmall;

uniform mat4 inverseV;
uniform vec4 projectionMatrix;

// Output: the fragment color
out vec3 fragColor;

#define SAMPLES_COUNT 16u
#define MAX_LOD 12

vec3 positionFromDepth(float depth){
	float depth2 = 2.0 * depth - 1.0 ;
	vec2 ndcPos = 2.0 * In.uv - 1.0;
	// Linearize depth -> in view space.
	float viewDepth = - projectionMatrix.w / (depth2 + projectionMatrix.z);
	// Compute the x and y components in view space.
	return vec3(- ndcPos * viewDepth / projectionMatrix.xy , viewDepth);
}

vec2 hammersleySample(uint i) {
	uint bits = i;
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float y = float(bits) * 2.3283064365386963e-10; // / 0x100000000
	return vec2(float(i)/SAMPLES_COUNT, y);
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

vec3 ggx(vec3 n, vec3 v, vec3 F0, float roughness){
	// Compute local frame.
	vec3 temp = abs(n.z) < 0.999 ? vec3(0.0,0.0,1.0) : vec3(1.0,0.0,0.0);
	vec3 tangent = normalize(cross(temp, n));
	vec3 binormal = cross(n, tangent);
	float alpha = max(0.0001, roughness*roughness);
	
	float NdotV = max(0.0001, abs(dot(v, n)));
	vec3 sum = vec3(0.0);
	
	for(uint i = 0u; i < SAMPLES_COUNT; ++i){
		// Draw a sample using Van der Corput sequence.
		vec2 sampleVec = hammersleySample(i);
		// Compute corresponding angles.
		float cosT2 = (1.0 - sampleVec.y)/(1.0+(alpha*alpha-1.0)*sampleVec.y);
		float cosT = sqrt(cosT2);
		float sinT = sqrt(1.0-cosT2);
		float angle = 2.0*M_PI*sampleVec.x;
		// Local half vector and light direction.
		vec3 h = sinT*cos(angle) * tangent + sinT*sin(angle) * binormal + cosT * n;
		vec3 l = -reflect(v,h);
		
		float NdotL = clamp(abs(dot(n,l)), 0.0001, 1.0);
		if(NdotL > 0.0){
			
			float VdotH = clamp(abs(dot(v,h)), 0.0001, 1.0);
			float NdotH = clamp(abs(dot(n,h)), 0.0001, 1.0);
			// Transform local light direction in world space.
			vec3 worldLight = normalize(vec3(inverseV * vec4(l,0.0)));
			// Estimate LOD based on roughness.
			float lodS = roughness < 0.03 ? 0.0 : max(0.0,
												   MAX_LOD - 1.5 - 0.5 * M_INV_LOG2 *
												   ( log(float(SAMPLES_COUNT))
												   + log((1.0 - l.y * l.y) * D(NdotH, alpha) * NdotH / (4.0 * VdotH))
												   ));
			
			// Read specular envmap.
			vec3 envLighting = textureLod(textureCubeMap, worldLight, lodS).rgb;
			// Add the sample contribution.
			sum += G(NdotL, NdotV, alpha) * VdotH * NdotL / NdotH * F(F0, VdotH) * envLighting;
		}
	}
	return sum/SAMPLES_COUNT;
}

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
	float depth = texture(depthTexture,In.uv).r;
	vec3 position = positionFromDepth(depth);
	vec3 n = normalize(2.0 * texture(normalTexture,In.uv).rgb - 1.0);
	vec3 v = normalize(-position);
	
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
	vec3 specular = ggx(n, v, F0, roughness);

	fragColor = ao * (diffuse + specular);
}
