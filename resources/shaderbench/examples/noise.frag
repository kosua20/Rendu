
layout(location = 0) in INTERFACE {
	vec4 dir; ///< View world direction.
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 1, binding = 3) uniform sampler2D noiseMap; ///< RGBA uniform noise in [0,1], uncorrelated.
layout(set = 1, binding = 5) uniform sampler2D directionsMap; ///< Random 3D directions on the unit sphere.
layout(set = 1, binding = 6) uniform sampler3D noise3DMap; ///< Random 3D directions on the unit sphere.

layout(set = 0, binding = 0) uniform UniformBlock {
	float iTime;
	vec3 iResolution;
	float scaleSpace;// = 10.0;
	float scaleTime;// = 2.0;
	int octaveCount;// = 4;
	float scaleExpansion;// = 2.0;
	float weightDecay;// = 0.5;
};

layout(location = 0) out vec4 fragColor; ///< Output color.


const int hashes[512] = int[](224, 169, 176, 19, 114, 187, 23, 16, 27, 252, 181, 88, 21, 198, 223, 57, 203, 119, 63, 13, 230, 56, 167, 41, 115, 75, 28, 251, 155, 47, 113, 59, 103, 144, 53, 178, 191, 46, 236, 205, 60, 141, 15, 66, 50, 58, 133, 190, 12, 197, 239, 69, 51, 138, 248, 124, 64, 164, 146, 165, 97, 204, 244, 235, 189, 195, 93, 5, 196, 32, 3, 183, 6, 219, 111, 44, 171, 149, 100, 38, 145, 229, 70, 162, 194, 54, 241, 127, 243, 22, 62, 199, 221, 140, 160, 0, 170, 92, 112, 82, 131, 123, 9, 154, 71, 253, 137, 172, 126, 233, 2, 238, 135, 84, 142, 242, 226, 86, 25, 201, 227, 87, 134, 143, 217, 83, 107, 210, 81, 118, 216, 247, 173, 153, 151, 214, 215, 206, 99, 246, 55, 207, 61, 213, 128, 184, 200, 231, 73, 139, 125, 208, 177, 36, 202, 186, 249, 108, 29, 179, 158, 136, 77, 45, 85, 163, 102, 212, 232, 209, 159, 30, 228, 240, 72, 43, 150, 10, 68, 245, 161, 192, 96, 48, 120, 98, 14, 104, 105, 166, 237, 101, 89, 106, 26, 90, 8, 250, 76, 78, 17, 193, 95, 91, 11, 132, 18, 185, 1, 94, 218, 24, 182, 31, 74, 109, 152, 234, 148, 157, 255, 254, 40, 33, 129, 175, 42, 117, 222, 156, 174, 116, 211, 122, 7, 168, 37, 49, 4, 65, 79, 188, 80, 34, 52, 130, 147, 180, 110, 35, 220, 121, 39, 225, 20, 67,
							  224, 169, 176, 19, 114, 187, 23, 16, 27, 252, 181, 88, 21, 198, 223, 57, 203, 119, 63, 13, 230, 56, 167, 41, 115, 75, 28, 251, 155, 47, 113, 59, 103, 144, 53, 178, 191, 46, 236, 205, 60, 141, 15, 66, 50, 58, 133, 190, 12, 197, 239, 69, 51, 138, 248, 124, 64, 164, 146, 165, 97, 204, 244, 235, 189, 195, 93, 5, 196, 32, 3, 183, 6, 219, 111, 44, 171, 149, 100, 38, 145, 229, 70, 162, 194, 54, 241, 127, 243, 22, 62, 199, 221, 140, 160, 0, 170, 92, 112, 82, 131, 123, 9, 154, 71, 253, 137, 172, 126, 233, 2, 238, 135, 84, 142, 242, 226, 86, 25, 201, 227, 87, 134, 143, 217, 83, 107, 210, 81, 118, 216, 247, 173, 153, 151, 214, 215, 206, 99, 246, 55, 207, 61, 213, 128, 184, 200, 231, 73, 139, 125, 208, 177, 36, 202, 186, 249, 108, 29, 179, 158, 136, 77, 45, 85, 163, 102, 212, 232, 209, 159, 30, 228, 240, 72, 43, 150, 10, 68, 245, 161, 192, 96, 48, 120, 98, 14, 104, 105, 166, 237, 101, 89, 106, 26, 90, 8, 250, 76, 78, 17, 193, 95, 91, 11, 132, 18, 185, 1, 94, 218, 24, 182, 31, 74, 109, 152, 234, 148, 157, 255, 254, 40, 33, 129, 175, 42, 117, 222, 156, 174, 116, 211, 122, 7, 168, 37, 49, 4, 65, 79, 188, 80, 34, 52, 130, 147, 180, 110, 35, 220, 121, 39, 225, 20, 67);

float dotGrad(ivec3 ip, vec3 dp){
	int id = hashes[hashes[hashes[ip.x] + ip.y] + ip.z];
	vec3 grad = texelFetch(directionsMap, ivec2(id/64, id%64), 0).rgb;
	return dot((grad), dp);
}

float perlin(vec3 p){
	vec3 x = p;
	ivec3 ix = ivec3(floor(x));
	vec3 dx = x - vec3(ix);
	ix &= (256-1);
	// Fetch cell gradients.
	ivec2 s = ivec2(1,0);
	vec4 g0s, g1s;
	g0s[0] = dotGrad(ix        , dx        );
	g0s[1] = dotGrad(ix + s.yxy, dx - s.yxy);
	g0s[2] = dotGrad(ix + s.yyx, dx - s.yyx);
	g0s[3] = dotGrad(ix + s.yxx, dx - s.yxx);
	g1s[0] = dotGrad(ix + s.xyy, dx - s.xyy);
	g1s[1] = dotGrad(ix + s.xxy, dx - s.xxy);
	g1s[2] = dotGrad(ix + s.xyx, dx - s.xyx);
	g1s[3] = dotGrad(ix + s.xxx, dx - s.xxx);
	// Compute weights.
	vec3 dx3 = dx * dx * dx;
	vec3 weights = ((6.0 * dx - 15.0) * dx + 10.0) * dx3;
	// Final value.
	vec4 gs = mix(g0s, g1s, weights.x);
	vec2 g = mix(gs.xz, gs.yw, weights.y);
	return mix(g.x, g.y, weights.z);
}

float worley(vec3 p){
	// Work in 2D for now.
	ivec3 ip = ivec3(floor(p));
	vec3 dp = p - vec3(ip);

	ivec3 pp = ip+1;
	ivec3 mp = ip-1;

	ivec3 clCoords = ivec3(floor(log2(textureSize(noise3DMap, 0).xyz)));
	ivec3 tp = ivec3(1) << clCoords;
	
	ivec3 wip[3];
	wip[1] = ivec3((ip) & (tp-1));
	wip[2] = ivec3((pp) & (tp-1));
	wip[0] = ivec3((mp) & (tp-1));

	float bestDist = 100000.0;
	for(int dz = -1; dz <= 1; ++dz){
		for(int dy = -1; dy <= 1; ++dy){
			for(int dx = -1; dx <= 1; ++dx){
				ivec3 pcoord = ivec3(wip[dx+1].x, wip[dy+1].y, wip[dz+1].z);
				vec3 rnd = texelFetch(noise3DMap, pcoord, 0).rgb;
				// Use the [0,1] random numbers as shift from the cell corner.
				vec3 loc = vec3(dx, dy, dz) + rnd;
				float dist = distance(loc, dp);
				bestDist = min(dist, bestDist);
			}
		}
	}
	return (bestDist / sqrt(2.0))*2.0-1.0;
}

float fbmP(vec3 p){
	float sum = 0.0;
	float scale = 1.0;
	float weight = 1.0;
	for(int i = 0; i < octaveCount; ++i){
		sum += weight * perlin(scale * p);
		scale *= scaleExpansion;
		weight *= weightDecay;
	}
	return sum;
}


float fbmW(vec3 p){
	float sum = 0.0;
	float scale = 1.0;
	float weight = 1.0;
	for(int i = 0; i < octaveCount; ++i){
		sum += weight * worley(scale * p);
		scale *= scaleExpansion;
		weight *= weightDecay;
	}
	return sum;
}

/// Main render function.
void main(){
	vec2 uv = (2.0*In.uv-1.0)*vec2(iResolution.x/iResolution.y, 1.0);
	float noise = 0.0;
	vec3 coords = vec3(scaleSpace * uv, scaleTime*iTime);
	if(uv.x < 0.0 && uv.y > 0.0){
		noise = perlin(coords);
	} else if(uv.x < 0.0){
		noise = worley(coords);
	} else if (uv.y > 0.0){
		noise = fbmP(coords);
	} else {
		noise = fbmW(coords);
	}
	fragColor.rgb = vec3(noise*0.5+0.5);
	fragColor.a = 1.0;
}
