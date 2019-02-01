#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;


#define INV_M_PI 0.3183098862
#define M_PI 3.1415926536
#define M_INV_LOG2 1.4426950408889
#define SAMPLE_COUNT 1024u

layout(location = 0) out vec2 fragColor; ///< BRDF linear coefficients.

/** Compute an arbitrary sample of the 2D Hammersley sequence.
\param i the index in the hammersley sequence
\return the i-th 2D sample
*/
vec2 hammersleySample(uint i) {
	uint bits = i;
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float y = float(bits) * 2.3283064365386963e-10; // / 0x100000000
	return vec2(float(i)/float(SAMPLE_COUNT), y);
}

/** Geometric half-term of GGX BRDF.
\param NdotX dot product of either the light or the view direction with the surface normal
\param halfAlpha half squared roughness
\return the value of the half-term
*/
float G1(float NdotX, float halfAlpha){
	return 1.0 / (NdotX * (1.0 - halfAlpha) + halfAlpha);
}

/** Geometric term of GGX BRDF, G.
\param NdotL dot product of the light direction with the surface normal
\param NdotV dot product of the view direction with the surface normal
\param alpha squared roughness
\return the value of G
*/
float G(float NdotL, float NdotV, float alpha){
	float halfAlpha = alpha * 0.5;
	return G1(NdotL, halfAlpha)*G1(NdotV, halfAlpha);
}

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
	vec3 binormal = cross(n, tangent);
	
	float alpha = max(0.000, roughness*roughness);

	vec2 sum = vec2(0.0);
	
	for(uint i = 0u; i < SAMPLE_COUNT; ++i){
		// Draw a sample using Van der Corput sequence.
		vec2 sampleVec = hammersleySample(i);
		
		// Compute corresponding angles.
		float cosT2 = (1.0 - sampleVec.y)/(1.0+(alpha*alpha-1.0)*sampleVec.y);
		float cosT = sqrt(cosT2);
		float sinT = sqrt(1.0-cosT2);
		
		float angle = 2.0*M_PI*sampleVec.x;
		// Local half vector and light direction.
		vec3 h = normalize(sinT*cos(angle) * tangent + sinT*sin(angle) * binormal + cosT * n);
		vec3 l = -reflect(v,h);
		
		float NdotL = max(l.z, 0.000);
		if(NdotL > 0.0){
			
			float VdotH = max(dot(v,h), 0.000);
			float NdotH = max(h.z, 0.000);
			
			float Gterm = G(NdotL, NdotV, alpha) * VdotH * NdotL / NdotH;
			float Fc = pow(2.0, (-5.55473 * VdotH - 6.98316) * VdotH);
			sum += vec2(1.0 - Fc, Fc) * Gterm;
			
		}
	}
	return sum/float(SAMPLE_COUNT);
}

/** Compute a linear approximation of the GGX BRDF in two coefficients.
*/
void main(){
	fragColor = ggx(In.uv.x,In.uv.y);
}
