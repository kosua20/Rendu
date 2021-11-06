#include "samplers.glsl"

#define PARALLAX_MIN 8
#define PARALLAX_MAX 96
#define PARALLAX_SCALE 0.03

/**
	Perform parallax mapping by marching against the local depth map, and output the final UV to use.
	\param uv the initial texture coordinates
	\param vTangentDir the view direction in tangent space
	\param depth the heightmap
	\param shiftDistance will contain the final shift along the view direction
	\return the final texture coordinates to use to query the material maps
*/
vec2 parallax(vec2 uv, vec3 vTangentDir, texture2D depth, out float shiftDistance){
	
	// We can adapt the layer count based on the view direction. If we are straight above the surface, we don't need many layers.
	float layersCount = mix(PARALLAX_MAX, PARALLAX_MIN, abs(vTangentDir.z));
	// Depth will vary between 0 and 1.
	float layerHeight = 1.0 / layersCount;
	float currentLayer = 0.0;
	// Apply a slight rescaling to the UVs to avoid issue at boundaries.
	uv = uv * 0.998 + 0.001;
	// Initial depth at the given position.
	float currentDepth = texture(sampler2D(depth, sRepeatLinearLinear), uv).r;
	
	// Step vector: in tangent space, we walk on the surface, in the (X,Y) plane.
	vec2 shift = PARALLAX_SCALE * vTangentDir.xy;
	// This shift corresponds to a UV shift, scaled depending on the height of a layer and the vertical coordinate of the view direction.
	// We have to take into account that our UVs are flipped vertically.
	vec2 shiftUV = shift / vTangentDir.z * layerHeight * vec2(1.0,-1.0);
	vec2 newUV = uv;
	
	// While the current layer is above the surface (ie smaller than depth), we march.
	while (currentLayer < currentDepth) {
		// We update the UV, going further away from the viewer.
		newUV -= shiftUV;
		// Update current depth.
		currentDepth = texture(sampler2D(depth, sRepeatLinearLinear), newUV).r;
		// Update current layer.
		currentLayer += layerHeight;
	}
	
	// Perform interpolation between the current depth layer and the previous one to refine the UV shift.
	vec2 previousNewUV = newUV + shiftUV;
	// The local depth is the gap between the current depth and the current depth layer.
	float currentLocalDepth = currentDepth - currentLayer;
	// Gap between the previous depth and the previous depth layer.
	float previousDepth = texture(sampler2D(depth, sRepeatLinearLinear), previousNewUV).r;
	float previousLocalDepth = previousDepth - (currentLayer - layerHeight);
	// Both of these allow use to estimate the slope of the depth between the two samples, and thus approximate the true intersection
	// which lies somewhere between the last two UV sets tested.
	float mixRatio = currentLocalDepth / (currentLocalDepth - previousLocalDepth);
	// Interpolate between the two local depths to obtain the correct UV shift.
	vec2 finalUV = mix(newUV, previousNewUV, mixRatio);
	float finalDepth = mix(currentDepth, previousDepth, mixRatio);

	// Ouptut distance covered along the view direction.
	shiftDistance = PARALLAX_SCALE * length( vec3((finalUV - uv) / layerHeight, finalDepth));
	return finalUV;
}

/** Compute the new view space position of the parallax-mapped fragment and update its depth in the depth buffer.
 \param shiftDistance the shift along the view direction
 \param viewPos the initial view space fragment position
 \param proj the projection matrix
 \return the updated position
 */
vec3 updateFragmentPosition(float shiftDistance, vec3 viewPos, mat4 proj){
	// For parallax mapping we have to update the depth of the fragment with the new found depth.
	// Store depth manually (see below).
	gl_FragDepth = gl_FragCoord.z;
	// Update the depth in view space.
	vec3 viewDir = normalize(viewPos);
	vec3 newViewSpacePosition = viewPos + shiftDistance * viewDir;
	// Back to clip space.
	vec4 clipPos = proj * vec4(newViewSpacePosition, 1.0);
	// Perspective division.
	float newDepth = clipPos.z / clipPos.w;
	// Update the fragment depth.
	gl_FragDepth = newDepth;
	/// \todo Depth output is sometimes incorrect.
	return newViewSpacePosition;
}
