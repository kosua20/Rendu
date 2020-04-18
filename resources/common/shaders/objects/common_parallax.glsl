
#define PARALLAX_MIN 8
#define PARALLAX_MAX 64
#define PARALLAX_SCALE 0.03

/**
	Perform parallax mapping by marching against the local depth map, and output the final UV to use.
	\param uv the initial texture coordinates
	\param vTangentDir the view direction in tangent space
	\param depth the heightmap
	\param positionShift will contain the final position shift
	\return the final texture coordinates to use to query the material maps
*/
vec2 parallax(vec2 uv, vec3 vTangentDir, sampler2D depth, out vec2 positionShift){
	
	// We can adapt the layer count based on the view direction. If we are straight above the surface, we don't need many layers.
	float layersCount = mix(PARALLAX_MAX, PARALLAX_MIN, abs(vTangentDir.z));
	// Depth will vary between 0 and 1.
	float layerHeight = 1.0 / layersCount;
	float currentLayer = 0.0;
	// Apply a slight rescaling to the UVs to avoid issue at boundaries.
	uv = uv * 0.998 + 0.001;
	// Initial depth at the given position.
	float currentDepth = texture(depth, uv).r;
	
	// Step vector: in tangent space, we walk on the surface, in the (X,Y) plane.
	vec2 shift = PARALLAX_SCALE * vTangentDir.xy;
	// This shift corresponds to a UV shift, scaled depending on the height of a layer and the vertical coordinate of the view direction.
	vec2 shiftUV = shift / vTangentDir.z * layerHeight;
	vec2 newUV = uv;
	
	// While the current layer is above the surface (ie smaller than depth), we march.
	while (currentLayer < currentDepth) {
		// We update the UV, going further away from the viewer.
		newUV -= shiftUV;
		// Update current depth.
		currentDepth = texture(depth, newUV).r;
		// Update current layer.
		currentLayer += layerHeight;
	}
	
	// Perform interpolation between the current depth layer and the previous one to refine the UV shift.
	vec2 previousNewUV = newUV + shiftUV;
	// The local depth is the gap between the current depth and the current depth layer.
	float currentLocalDepth = currentDepth - currentLayer;
	float previousLocalDepth = texture(depth, previousNewUV).r - (currentLayer - layerHeight);
	
	
	// Interpolate between the two local depths to obtain the correct UV shift.
	vec2 finalUV = mix(newUV, previousNewUV, currentLocalDepth / (currentLocalDepth - previousLocalDepth));
	positionShift = (uv - finalUV) * vTangentDir.z / layerHeight;
	return finalUV;
}

/** Compute the new view space position of the parallax-mapped fragment and update its depth in the depth buffer.
 \param localUV the final computed UV
 \param positionShift the shift in the tangent plane
 \param viewPos the initial view space fragment position
 \param proj the projection matrix
 \param tbn the tangent space to view space matrix
 \param depth the heightmap used for parallax mapping
 \return the updated position
 */
vec3 updateFragmentPosition(vec2 localUV, vec2 positionShift, vec3 viewPos, mat4 proj, mat3 tbn, sampler2D depth){
	// For parallax mapping we have to update the depth of the fragment with the new found depth.
	// Store depth manually (see below).
	gl_FragDepth = gl_FragCoord.z;
	// Update the depth using the heightmap and the displacement applied.
	// Read the depth.
	float localDepth = texture(depth, localUV).r;
	// Convert the 3D shift applied from tangent space to view space.
	vec3 shift = tbn * vec3(positionShift, -PARALLAX_SCALE * localDepth);
	// Update the depth in view space.
	vec3 viewDir = normalize(viewPos);
	vec3 newViewSpacePosition = viewPos - dot(shift, viewDir) * viewDir;
	// Back to clip space.
	vec4 clipPos = proj * vec4(newViewSpacePosition, 1.0);
	// Perspective division.
	float newDepth = clipPos.z / clipPos.w;
	// Update the fragment depth, taking into account the depth range parameters.
	gl_FragDepth = ((gl_DepthRange.diff * newDepth) + gl_DepthRange.near + gl_DepthRange.far)/2.0;
	return newViewSpacePosition;
}
