
#include "common_pbr.glsl"
#include "utils.glsl"

layout(location = 0) in INTERFACE {
	vec3 pos;  ///< World position.
} In ;

layout(set = 2, binding = 0) uniform textureCube texture0; ///< Input cubemap to process.

layout(set = 0, binding = 0) uniform UniformBlock {
	float mipmapRoughness; ///< The roughness to use for the convolution lobe.
	float clampMax; ///< Clamp input HDR values to avoid artefacts.
	int samplesCount; ///< Number of samples to take, higher count helps avoiding artifacts in bright areas.
};

layout(location = 0) out vec3 fragColor; ///< Color.


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
	vec3 bitangent = cross(n, tangent);

	float alpha = max(0.000, roughness*roughness);

	vec3 sum = vec3(0.0);
	float denom = 0.0;
	for(int i = 0; i < samplesCount; ++i){
		// Draw a sample using Van der Corput sequence.
		vec2 sampleVec = hammersleySample(uint(i), samplesCount);

		// Compute corresponding angles.
		float cosT2 = (1.0 - sampleVec.y)/(1.0+(alpha*alpha-1.0)*sampleVec.y);
		float cosT = sqrt(cosT2);
		float sinT = sqrt(1.0-cosT2);

		float angle = 2.0*M_PI*sampleVec.x;
		// Local half vector and light direction.
		vec3 h = normalize(sinT*cos(angle) * tangent + sinT*sin(angle) * bitangent + cosT * n);
		vec3 l = normalize(-reflect(v,h));

		float NdotL = max(dot(n,l), 0.000);
		if(NdotL > 0.0){
			sum += NdotL * clamp(textureLod(samplerCube(texture0, sClampLinear), toCube(l), 0.0).rgb, 0.0, clampMax);
			denom += NdotL;
		}
	}
	return sum/denom;
}

/** Perform convolution of the cubemap with the BRDF lobe at a given roughness, using a high number of samples 
  and importance sampling. This is done in the direction in world space of the current fragment.
*/
void main(){
	fragColor = convo(normalize(In.pos), mipmapRoughness);
}
