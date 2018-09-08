#version 330

// Input: UV coordinates
in INTERFACE {
	vec3 pos;
} In ; ///< vec3 pos;

#define INV_M_PI 0.3183098862
#define M_PI 3.1415926536
#define M_INV_LOG2 1.4426950408889
#define SAMPLE_COUNT 10000u // Super high sample count to avoid artifacts in bright areas.

layout(binding = 0) uniform samplerCube texture0; ///< Input cubemap to process.
uniform float mimapRoughness; ///< The roughness to use for the convolution lobe.

layout(location = 0) out vec3 fragColor; ///< Color.

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

/** Perform convolution with the BRDF specular lobe, evaluated in a given direction. 
\param r the reflection direction
\param roughness the roughness to evalute the BRDF at
\return the result of the convolution
*/
vec3 convo(vec3 r, float roughness){
	// Fix view and normal as in the Unreal white paper.
	vec3 v = r;
	vec3 n = r;
	
	// Compute local frame.
	vec3 temp = abs(n.z) < 0.999 ? vec3(0.0,0.0,1.0) : vec3(1.0,0.0,0.0);
	vec3 tangent = normalize(cross(temp, n));
	vec3 binormal = cross(n, tangent);
	
	float alpha = max(0.000, roughness*roughness);
	
	vec3 sum = vec3(0.0);
	float denom = 0.0;
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
		vec3 l = normalize(-reflect(v,h));
		
		float NdotL = max(dot(n,l), 0.000);
		if(NdotL > 0.0){
			sum += NdotL * texture(texture0, l).rgb;
			denom += NdotL;
		}
	}
	return sum/denom;
}

/** Perform convolution of the cubemap with the BRDF lobe at a given roughness, using a high number of samples 
  and importance sampling. This is done in the direction in world space of the current fragment.
*/
void main(){
	fragColor = convo(normalize(In.pos), mimapRoughness);
}
