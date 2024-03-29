
#include "common_pbr.glsl"
#include "utils.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

#define SAMPLE_COUNT 1024u
#define SAMPLE_COUNT_SHEEN 4096u

layout(location = 0) out vec4 fragColor; ///< BRDF linear coefficients.

/** Evaluated the GGX BRDF for a given surface normal, view direction and roughness.
\param NdotV dot product of the view direction with the surface normal
\param roughness the roughness of the surface
\return the corresponding value of the BRDF
*/
vec3 ggx(float NdotV, float roughness){
	
	vec3 v = vec3(sqrt( 1.0f - NdotV * NdotV ), 0.0, NdotV);
	vec3 n = vec3(0.0,0.0,1.0);
	
	// Compute local frame.
	vec3 temp = abs(n.z) < 0.999 ? vec3(0.0,0.0,1.0) : vec3(1.0,0.0,0.0);
	vec3 tangent = normalize(cross(temp, n));
	vec3 bitangent = cross(n, tangent);
	
	float alpha = max(0.0001, roughness*roughness);
	float invAlpha = 1.0 / alpha;

	vec3 sum = vec3(0.0);

	// Importance sampling of standard isotropic GGX BRDF.
	// Compute corresponding angles.	
	for(uint i = 0u; i < SAMPLE_COUNT; ++i){
		// Draw a sample using Van der Corput sequence.
		vec2 sampleVec = hammersleySample(i, int(SAMPLE_COUNT));
		float angle = 2.0 * M_PI * sampleVec.x;

		float cosT2 = (1.0 - sampleVec.y)/(1.0+(alpha*alpha-1.0)*sampleVec.y);
		float cosT = sqrt(cosT2);
		float sinT = sqrt(1.0-cosT2);

		// Local half vector and light direction.
		vec3 h = normalize(sinT*cos(angle) * tangent + sinT*sin(angle) * bitangent + cosT * n);
		vec3 l = -reflect(v, h);

		float NdotL = max(l.z, 0.000);
		if(NdotL > 0.0){

			float VdotH = max(dot(v, h), 0.000);
			float NdotH = max(h.z, 0.000);

			float Gterm =  4.0 * V(NdotL, NdotV, alpha) * NdotL * VdotH / NdotH;
			float Fc = pow(1.0 - VdotH, 5.0);
			sum.xy += Gterm * vec2(1.0 - Fc, Fc);
		}

	}
	sum.xy /= float(SAMPLE_COUNT);

	// Uniform sampling of sheen BRDF, as described by R. Guy and M. Agopian in "Physically Based Rendering
	// in Filament", 2021, (https://google.github.io/filament/Filament.md.html#lighting/imagebasedlights/cloth)	
	for(uint i = 0u; i < SAMPLE_COUNT_SHEEN; ++i){
		// Draw a sample using Van der Corput sequence.
		vec2 sampleVec = hammersleySample(i, int(SAMPLE_COUNT_SHEEN));
		float angle = 2.0 * M_PI * sampleVec.x;

		// Compute corresponding angles.
		float cosT = 1.0 - sampleVec.y;
		float sinT = sqrt(1.0 - cosT * cosT);

		// Local half vector and light direction.
		vec3 h = normalize(sinT*cos(angle) * tangent + sinT*sin(angle) * bitangent + cosT * n);
		vec3 l = -reflect(v, h);

		float NdotL = max(l.z, 0.000);
		if(NdotL > 0.0){

			float VdotH = max(dot(v, h), 0.000);
			float NdotH = max(h.z, 0.000);
			float sinH2 = max(1.0 - NdotH * NdotH, 0.0001);

			float Vterm = 1.0 / (4.0 * (NdotL + NdotV - NdotL * NdotV));
			float Dterm = (2.0 + invAlpha) * pow(sinH2, 0.5 * invAlpha) * 0.5 * INV_M_PI;
			sum.z += Vterm * Dterm * NdotL * VdotH;
		}
	}
	sum.z *= 4.0 * 2.0 * M_PI / float(SAMPLE_COUNT_SHEEN);

	return sum; 
}

/** Compute a linear approximation of the GGX BRDF in two coefficients.
*/
void main(){
	fragColor = vec4(ggx(In.uv.x, In.uv.y), 1.0);
}
