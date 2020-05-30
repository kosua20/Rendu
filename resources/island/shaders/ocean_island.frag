#version 400

#include "gerstner_waves.glsl"

in INTERFACE {
	vec3 pos;
	vec3 srcPos;
} In ;

uniform vec3 camPos;
uniform vec3 lightDirection;
uniform bool debugCol;
uniform float time;
uniform float texelSize;
uniform float invMapSize;
uniform bool raycast;
uniform vec2 invTargetSize;


layout(binding = 0) uniform sampler2D heightMap;
layout(binding = 1) uniform sampler2D terrainColor;
layout(binding = 2) uniform sampler2D terrainPos;
layout(binding = 3) uniform sampler2D terrainColorBlur;
layout(binding = 4) uniform sampler2D absorpScatterLUT;
layout(binding = 5) uniform sampler2D waveNormals;
layout(binding = 7) uniform samplerCube envmap;

layout(std140, binding = 0) uniform Waves {
	Wave waves[8];
};

layout (location = 0) out vec4 fragColor;

const vec3 sunColor = vec3(1.474, 1.8504, 1.91198);

float fresnelWater(float NdotV){
	float F0 = (1.0-1.33)/(1.0 + 1.33);
	F0 *= F0;
	return F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);
}

/** Shade the object, applying lighting. */
void main(){

	vec3 worldPos;
	vec3 srcPos;
	float viewDist;
	vec3 vdir;

	if(raycast){
		// For the distant cylinder mesh, raycast from the camera and intersect the ocean plane.
		vec3 rayDir = normalize(In.pos - camPos);
		if(rayDir.y >= -0.001){
			discard;
		}
		float lambda = -camPos.y / rayDir.y;
		worldPos = camPos + lambda * rayDir;
		viewDist = lambda;
		vdir = rayDir;
		srcPos = worldPos;
	} else {
		// For the tessellated grid just use the input info.
		worldPos = In.pos;
		vec3 dView = worldPos - camPos;
		viewDist = length(dView);
		vdir = dView/max(viewDist, 0.001);
		srcPos = In.srcPos;
	}

	vec3 nn = vec3(0.0);
	vec3 tn = vec3(0.0);
	vec3 bn = vec3(0.0);

	// Compute waves normal, applying a fade out when in the distance to avoid aliasing.
	float dist2 = viewDist*viewDist;
	float adjust = 1000.0f;
	for(int i = 7; i >= 0; --i){
		// Fade out high frequencies when far away.
		float distWeight = exp(-dist2*pow(i+0.5, 2.0)/adjust);
		gerstnerFrame(waves[i], worldPos, time, tn, bn, nn, distWeight);
	}
	tn.z += 1.0;
	bn.x += 1.0;
	nn.y += 1.0;
	nn = normalize(nn);
	bn = normalize(bn);
	tn = normalize(tn);
	mat3 tbn = mat3(tn, bn, nn);
	// Read high frequency normal map based on undistorted world position.
	vec2 warpUV = 2.0*srcPos.xz;
	vec3 warpN = texture(waveNormals, warpUV, log2(viewDist)).xyz * 2.0 - 1.0;
	vec3 n = normalize(tbn * warpN);//mix(warpN, vec3(0.0,0.0,1.0), clamp(viewDist/10.0, 0.0, 1.0)));

	vec2 screenUV = (gl_FragCoord.xy)*invTargetSize;
	// Compute length of the ray underwater.
	vec3 oceanFloorPos = textureLod(terrainPos, screenUV, 0.0).xyz;
	float distPos = distance(oceanFloorPos, worldPos);
	// Put the floor at 1.0 unit below approx.
	float scalingDist = 1.0/2.0;
	float distUnderWater = clamp(distPos * scalingDist, 0.0, 1.0);

	// Blend blurred version of the floor in deeper regions.
	vec3 oceanFloor = textureLod(terrainColor, screenUV, 0.0).rgb;
	vec3 oceanFloorBlur = textureLod(terrainColorBlur, screenUV, 0.0).rgb;
	vec3 floorColor = mix(oceanFloor, oceanFloorBlur, distUnderWater);

	// Lookup absorption and scattering values for the given distance under water.
	vec3 absorp = textureLod(absorpScatterLUT, vec2(distUnderWater, 0.75), 0.0).rgb;
	vec3 scatter = textureLod(absorpScatterLUT, vec2(distUnderWater, 0.25), 0.0).rgb;
	vec3 baseColor = absorp * mix(floorColor, scatter, distUnderWater);

	// Apply a basic Phong lobe for now.
	vec3 ldir = reflect(vdir, n);
	// Fetch from envmap for now.
	vec3 reflection = texture(envmap, ldir).rgb;

	float NdotV = max(0.0, dot(n, -vdir));
	vec3 color = baseColor + fresnelWater(NdotV) * reflection;
	if(debugCol){
		color = vec3(0.9);
	}
	fragColor = vec4(color,1.0);
}
