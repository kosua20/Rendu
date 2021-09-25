#include "utils.glsl"
#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D depthTexture; ///< Depth texture.
layout(set = 2, binding = 1) uniform texture2D normalTexture; ///< Normal texture, in [0,1].
layout(set = 2, binding = 2) uniform texture2D noiseTexture; ///< 5x5 3-components noise texture with float precision.

layout(set = 0, binding = 0) uniform UniformBlock {
	mat4 projectionMatrix; ///< The camera projection parameters.
	float radius; ///< The sampling radius.
};

/// Unique sample directions on a sphere.
layout(std140, set = 3, binding = 0) uniform Samples {
	vec4 samples[16];
};

layout(location = 0) out float fragColor; ///< SSAO.

/** Estimate the screen space ambient occlusion in the scene. */
void main(){
	
	vec3 n = normalize(2.0 * texture(sampler2D(normalTexture, sClampLinear),In.uv).rgb - 1.0);
	
	// If normal is null, this is an emissive object (or background), no AO
	// (avoid reading the albedo texture just for the id.)
	if(length(n) < 0.1){
		fragColor = 1.0;
		return;
	}
	
	// Read the random local shift, uvs based on pixel coordinates (wrapping enabled).
	vec3 randomOrientation = texture(sampler2D(noiseTexture, sRepeatLinear), gl_FragCoord.xy/5.0).rgb;
	
	// Create tangent space to view space matrix by computing tangent and bitangent.
	vec3 t = normalize(randomOrientation - n * dot(randomOrientation, n));
	vec3 b = normalize(cross(n, t));
	mat3 tbn = mat3(t, b, n);

	// Compute the x and y components in view space.
	vec4 projParams = vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	vec3 position = positionFromDepth(texture(sampler2D(depthTexture, sClampLinear), In.uv).r, In.uv, projParams);

	
	// Occlusion accumulation.
	float occlusion = 0.0;
	for(int i = 0; i < 16; ++i){
		// View space position of the sample.
		vec3 randomSample = position + radius * tbn * samples[i].xyz;
		// Project view space point to clip space then NDC space.
		vec4 sampleClipSpace = projectionMatrix * vec4(randomSample, 1.0);
		vec2 sampleUV = (sampleClipSpace.xy / sampleClipSpace.w) * 0.5 + 0.5;
		// Read scene depth at the corresponding UV.
		float sampleDepth = linearizeDepth(texture(sampler2D(depthTexture, sClampLinear), sampleUV).r, projParams.zw);

		// Check : if the depth are too different, don't take result into account.
		float isValid = abs(position.z - sampleDepth) < radius ? 1.0 : 0.0;
		// If the initial sample is further away from the camera than the surface, it is below the surface, occlusion is increased.
		occlusion += (sampleDepth >= randomSample.z  ? isValid : 0.0);
	}
	
	// Normalize and reverse occlusion.
	occlusion = 1.0 - (occlusion/16.0);
	fragColor = occlusion;
}
