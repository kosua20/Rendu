#version 400

#include "gerstner_waves.glsl"

in INTERFACE {
	vec3 pos;
	vec3 srcPos;
	vec3 prevPos;
} In ;

uniform vec3 camPos;
uniform bool debugCol;
uniform float time;
uniform bool distantProxy;
uniform vec2 invTargetSize;

layout(binding = 0) uniform sampler2D foamMap;
layout(binding = 1) uniform sampler2D terrainColor;
layout(binding = 2) uniform sampler2D terrainPos;
layout(binding = 3) uniform sampler2D terrainColorBlur;
layout(binding = 4) uniform sampler2D absorpScatterLUT;
layout(binding = 5) uniform sampler2D waveNormals;
layout(binding = 6) uniform samplerCube envmap;

layout(std140, binding = 0) uniform Waves {
	Wave waves[8];
};

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec3 fragWorldPos;

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
	vec3 prevPos;
	vec3 vdir;

	if(distantProxy){
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
		prevPos = worldPos;
	} else {
		// For the tessellated grid just use the input info.
		worldPos = In.pos;
		vec3 dView = worldPos - camPos;
		viewDist = length(dView);
		vdir = dView/max(viewDist, 0.001);
		srcPos = In.srcPos;
		prevPos = In.prevPos;
	}
	fragWorldPos = worldPos;

	vec3 nn = vec3(0.0);
	vec3 tn = vec3(0.0);
	vec3 bn = vec3(0.0);

	// Compute waves normal, applying a fade out when in the distance to avoid aliasing.
	float dist2 = viewDist*viewDist;
	float adjust = 1000.0f;
	// We use the same split as when displacing the vertex.
	float biasGerstner = 0.75;
	for(int i = 7; i > 2; --i){
		// Fade out high frequencies when far away.
		float distWeight = exp(-dist2*pow(i+biasGerstner, 2.0)/adjust);
		gerstnerFrame(waves[i], srcPos, time, tn, bn, nn, distWeight);
	}
	for(int i = 2; i >= 0; --i){
		// Fade out high frequencies when far away.
		float distWeight = exp(-dist2*pow(i+biasGerstner, 2.0)/adjust);
		gerstnerFrame(waves[i], prevPos, time, tn, bn, nn, distWeight);
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
	vec3 n = normalize(tbn * warpN);

	vec3 floorColor = vec3(0.0);
	float distUnderWater = 1.0;
	if(!distantProxy){
		// Perturb ocean floor UVs based on normal and water depth.
		vec2 screenUV = (gl_FragCoord.xy)*invTargetSize;
		vec3 oceanFloorPosInit = textureLod(terrainPos, screenUV, 0.0).xyz;
		float distPosInit = distance(oceanFloorPosInit, worldPos);
		screenUV += 0.05 * n.xz * min(1.0, 0.1+distPosInit);
		// Compute length of the ray underwater.
		vec3 oceanFloorPos = textureLod(terrainPos, screenUV, 0.0).xyz;
		float distPos = distance(oceanFloorPos, worldPos);
		// Put the floor at 1.0 unit below approx.
		float scalingDist = 1.0/2.0;
		distUnderWater = clamp(distPos * scalingDist, 0.0, 1.0);

		// Blend blurred version of the floor in deeper regions.
		vec3 oceanFloor = textureLod(terrainColor, screenUV, 0.0).rgb;
		vec3 oceanFloorBlur = textureLod(terrainColorBlur, screenUV, 0.0).rgb;
		floorColor = mix(oceanFloor, oceanFloorBlur, distUnderWater);
	}

	// Lookup absorption and scattering values for the given distance under water.
	vec3 absorp = textureLod(absorpScatterLUT, vec2(distUnderWater, 0.75), 0.0).rgb;
	vec3 scatter = textureLod(absorpScatterLUT, vec2(distUnderWater, 0.25), 0.0).rgb;
	vec3 baseColor = absorp * mix(floorColor, scatter, distUnderWater);

	// Add underwater bubbles, using the same foam texture but at a lower LOD to blur (see CREST presentation).
	float NdotV = max(0.0, dot(n, -vdir));
	if(!distantProxy){
		vec2 bubblesUV = mix(srcPos.xz, worldPos.xz, 0.7) + 0.05 * time * vec2(0.5, 0.8) + 0.02 * n.xz;
		bubblesUV += 0.1 * vdir.xz / NdotV;
		vec4 bubbles = texture(foamMap, 0.5*bubblesUV, 2.0);
		baseColor += bubbles.a * bubbles.rgb / max(1.0, viewDist);
	}

	// Apply a basic Phong lobe for now.
	vec3 ldir = reflect(vdir, n);
	ldir.y = max(0.001, ldir.y);
	// Fetch from envmap for now.
	// Clamp to avoid fireflies.
	vec3 reflection = min(texture(envmap, ldir).rgb, 5.0);
	// Combine specular and fresnel.
	vec3 color = baseColor + fresnelWater(NdotV) * reflection;

	if(!distantProxy){
		// Apply foam on top of the shaded water.
		float foamAtten = 1.0-clamp(distUnderWater*10.0, 0.0, 1.0);
		foamAtten += (1.0-abs(n.y))/max(1.0, viewDist/2.0);
		foamAtten = sqrt(min(foamAtten, 1.0));
		vec2 foamUV = 1.5 * srcPos.xz + 0.02 * time;
		vec3 foam = texture(foamMap, foamUV).rgb;
		color += foamAtten * foam;
	}

	if(debugCol){
		color = vec3(0.9);
	}
	fragColor = color;
}
