/** Gerstener wave packed parameters. */
struct Wave {
	vec4 DiAngleActive; ///< 2D direction, corresponding angle, active flag.
	vec4 Aqwp; ///< Gerstner parameters.
};

/** Apply a Gerstner wave to a given location in the plane.
 \param gw the wave parameters
 \param p the 2D position to evaluate the wave at
 \param t the current time
 \return the shifted 3D wave position
 */
vec3 gerstner(Wave gw, vec2 p, float t){
	vec2 Di = gw.DiAngleActive.xy;
	float Ai = gw.Aqwp.x;
	float Qi = gw.Aqwp.y;
	float wi = gw.Aqwp.z;
	float phi = gw.Aqwp.w;
	// Compute position.
	float angle = wi * dot(Di, p) + phi * t;
	vec2 horiz = cos(angle) * Qi * Ai * Di.xy ;
	float vert = Ai * sin(angle);
	return float(gw.DiAngleActive.w > 0.001) * vec3(horiz.x, vert, horiz.y);
}

/** Compute the normal and tangent frame information associated to a given wave.
 \note Multiple waves can be applied sequentially, call gerstnerFrameFinalize at the end.
 \param gw the wave parameters
 \param pos the 3D wave position
 \param t the current time
 \param tn will contain accumulated tangent information
 \param bn will contain accumulated bitangent information
 \param nn will contain accumulated normal information
 \param distWeight wave attenuation factor
 \return the shifted 3D wave position
*/
void gerstnerFrame(Wave gw, vec3 pos, float t, inout vec3 tn, inout vec3 bn, inout vec3 nn, float distWeight){
	vec2 Di = gw.DiAngleActive.xy;
	float Ai = gw.Aqwp.x;
	float Qi = gw.Aqwp.y;
	float wi = gw.Aqwp.z;
	float phi = gw.Aqwp.w;

	float angle0 = wi * dot(Di, pos.xz) + phi * t;
	float wa = distWeight * float(gw.DiAngleActive.w > 0.001) * wi * Ai;
	float s0 = wa * sin(angle0);
	float c0 = wa * cos(angle0);

	// Compute tangent.
	tn.x += -Qi * Di.x * Di.y * s0;
	tn.z += /*1*/ -Qi * Di.y * Di.y * s0;
	tn.y += Di.y * c0;

	// Compute bitangent
	bn.x += /*1*/ -Qi * Di.x * Di.x * s0;
	bn.y += Di.x * c0;
	bn.z += -Qi * Di.x * Di.y * s0;

	// Compute normal.
	nn.xz += -c0 * Di.xy;
	nn.y += /*1*/ -Qi * s0;
}

/** Finalize the local tangent frame for a point on the surface.
 \param tn accumulated tangent information
 \param bn accumulated bitangent information
 \param nn accumulated normal information
 \note Inputs will be updated and normalized.
 */
void gerstnerFrameFinalize(inout vec3 tn, inout vec3 bn, inout vec3 nn){
	tn.z += 1.0;
	bn.x += 1.0;
	nn.y += 1.0;
	nn = normalize(nn);
	bn = normalize(bn);
	tn = normalize(tn);
}
