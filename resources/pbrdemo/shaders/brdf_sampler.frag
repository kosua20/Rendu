
#include "common_pbr.glsl"
#include "utils.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

#define SAMPLE_COUNT 1024u

layout(location = 0) out vec2 fragColor; ///< BRDF linear coefficients.

/** Evaluated the GGX BRDF for a given surface normal, view direction and roughness.
\param NdotV dot product of the view direction with the surface normal
\param roughness the roughness of the surface
\return the corresponding value of the BRDF
*/
vec2 ggx(float NdotV, float roughness){
	
	vec3 v = vec3(sqrt( 1.0f - NdotV * NdotV ), 0.0, NdotV);
	vec3 n = vec3(0.0,0.0,1.0);
	
	// Compute local frame.
	vec3 temp = abs(n.z) < 0.999 ? vec3(0.0,0.0,1.0) : vec3(1.0,0.0,0.0);
	vec3 tangent = normalize(cross(temp, n));
	vec3 bitangent = cross(n, tangent);
	
	float alpha = max(0.0001, roughness*roughness);

	vec2 sum = vec2(0.0);
	
	for(uint i = 0u; i < SAMPLE_COUNT; ++i){
		// Draw a sample using Van der Corput sequence.
		vec2 sampleVec = hammersleySample(i, int(SAMPLE_COUNT));
		
		// Compute corresponding angles.
		float cosT2 = (1.0 - sampleVec.y)/(1.0+(alpha*alpha-1.0)*sampleVec.y);
		float cosT = sqrt(cosT2);
		float sinT = sqrt(1.0-cosT2);
		
		float angle = 2.0*M_PI*sampleVec.x;
		// Local half vector and light direction.
		vec3 h = normalize(sinT*cos(angle) * tangent + sinT*sin(angle) * bitangent + cosT * n);
		vec3 l = -reflect(v,h);
		
		float NdotL = max(l.z, 0.000);
		if(NdotL > 0.0){
			
			float VdotH = max(dot(v,h), 0.000);
			float NdotH = max(h.z, 0.000);
			
			float Gterm =  4.0 * V(NdotL, NdotV, alpha) * NdotL * VdotH / NdotH ;
			float Fc = pow(1.0 - VdotH, 5.0);
			sum += Gterm * vec2(1.0 - Fc, Fc);
			
		}
	}
	return sum/float(SAMPLE_COUNT);
}

/** Compute a linear approximation of the GGX BRDF in two coefficients.
*/
void main(){
	fragColor = ggx(In.uv.x,In.uv.y);
}
