#version 330

// Input: UV coordinates
in INTERFACE {
	vec3 pos;
} In ;

#define INV_M_PI 0.3183098862
#define M_PI 3.1415926536
#define M_INV_LOG2 1.4426950408889
#define SAMPLE_COUNT 10000u // Super high sample count to avoid artifacts in bright areas.

uniform samplerCube texture0;

// Output: the fragment color
out vec3 fragColor;

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


void main(){
	fragColor = convo(normalize(In.pos), 0.8);
}
