
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
 \param layer the array layer index
 \return the direction to use to sample in the cubemap
 */
vec4 toCube(vec3 v, int layer){
	return vec4(v.x, -v.y, v.z, float(layer));
}

/** Store a 32bits float in a 4x8bits vector of normalized values.
\param f the float to store
\return the vec4 representation of the float
 */
vec4 floatToVec4(float f){
	uint raw = floatBitsToUint(f);
	vec4 res;
	res[0] = float((raw >>  0) & 0xFF) / 255.0;
	res[1] = float((raw >>  8) & 0xFF) / 255.0;
	res[2] = float((raw >> 16) & 0xFF) / 255.0;
	res[3] = float((raw >> 24) & 0xFF) / 255.0;
	return res;
}

/** Load a 32bits float from a 4x8bits vector of normalized values.
\param v the vec4 representation of the float
\return the loaded float
 */
float vec4ToFloat(vec4 v){
	uint res = 0;
	res |= (uint(v[0]*255.0) & 0xFF) << 0;
	res |= (uint(v[1]*255.0) & 0xFF) << 8;
	res |= (uint(v[2]*255.0) & 0xFF) << 16;
	res |= (uint(v[3]*255.0) & 0xFF) << 24;
	return uintBitsToFloat(res);
}

/** Wrap a projected direction in the octahedral parametrization of the unit sphere
 \param d the projected direction in 2D
 \return the corresponding coordinates in the parameterization
 */
vec2 octahedralWrap(vec2 d){
	return (1.0 - abs(d.yx)) * vec2(d.x >= 0.0 ? 1.0 : -1.0, d.y >= 0.0 ? 1.0 : -1.0);
}

/** Encode a normalized direction into an octahedral representation, as described in
	Octahedron normal vector encoding, Krzysztof Narkowicz, 2014
	(https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/)
 \param d the normalized direction
 \return the encoded representation
 */
vec2 encodeOctahedralDirection(vec3 d){
	d /= (abs(d.x) + abs(d.y) + abs(d.z));
	d.xy = d.z >= 0.0 ? d.xy : octahedralWrap(d.xy);
	return d.xy * 0.5 + 0.5;
}

/** Decode a normalized direction from an octahedral representation.
 \param e the representation parameters
 \return the decoded direction
 */
vec3 decodeOctahedralDirection(vec2 e){
	e = 2.0 * e - 1.0;
	vec3 d = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	float t = min(max(-d.z, 0.0), 1.0);
	vec2 dd = vec2(d.x >= 0.0 ? -t : t, d.y >= 0.0 ? -t : t);
	d.xy += dd;
	return normalize(d);
}

/** Encode a normal into a compact representation for storage in a framebuffer.
 \param n the normal to encode
 \return the encoded representation
 */
vec2 encodeNormal(vec3 n){
	// Octahedral encoding.
	return encodeOctahedralDirection(n);
}

/** Decode a normal from its storage-efficient representation.
 \param e the normal representation
 \return the decoded normal
 */
vec3 decodeNormal(vec2 e){
	return decodeOctahedralDirection(e);
}

