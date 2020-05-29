
struct Wave {
	vec4 DiAngleActive;
	vec4 Aqwp;
};

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

void gerstnerFrame(Wave gw, vec3 pos, float t, out vec3 tn, inout vec3 bn, inout vec3 nn, float distWeight){
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
