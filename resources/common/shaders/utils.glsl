
/** Linearize a NDC space to view space using the camera projection parameters. 
\param depth the non-linear depth of the fragment
\param projPlanes the depth-related coefficients of the projection matrix
\return the view space depth
*/
float linearizeDepth(float depth, vec2 projPlanes){
	float depth2 = depth;
	return -projPlanes.y / (depth2 + projPlanes.x);
}

/** Estimate the position of the current fragment in view space based on its depth and camera parameters.
\param depth the depth of the fragment
\param uv the input uv
\param projMat the non trivial coefficients of the projection matrix
\return the view space position
*/
vec3 positionFromDepth(float depth, vec2 uv, vec4 projMat){
	// Linearize depth -> in view space.
	float viewDepth = linearizeDepth(depth, projMat.zw);
	// Compute the x and y components in view space.
	vec2 ndcPos = 2.0 * uv - 1.0;
	return vec3(-ndcPos * viewDepth / projMat.xy , viewDepth);
}

/** Compute an arbitrary sample of the 2D Hammersley sequence.
\param i the index in the hammersley sequence
\param scount the total number of samples
\return the i-th 2D sample
*/
vec2 hammersleySample(uint i, int scount) {
	uint bits = i;
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float y = float(bits) * 2.3283064365386963e-10; // / 0x100000000
	return vec2(float(i)/float(scount), y);
}

/** Transform a world space direction to cubemap space. A vertical flip is needed.
 \param v the direction to flip
 \return the direction to use to sample in the cubemap
 */
vec3 toCube(vec3 v){
	return vec3(v.x, -v.y, v.z);
}

/** Transform a world space direction to cubemap array space. A vertical flip is needed.
 \param v the direction to flip
 \return the direction to use to sample in the cubemap
 */
vec4 toCube(vec3 v, int layer){
	return vec4(v.x, -v.y, v.z, float(layer));
}
