#version 400

#include "gerstner_waves.glsl"

in INTERFACE {
	vec3 pos;
} In ;

uniform vec3 camPos;
uniform vec3 lightDirection;
uniform bool debugCol;
uniform float time;
uniform float texelSize;
uniform float invMapSize;
uniform bool raycast;

layout(binding = 0) uniform sampler2D heightMap;

layout(std140, binding = 0) uniform Waves {
	Wave waves[8];
};

layout (location = 0) out vec4 fragColor;

const vec3 sunColor = vec3(1.474, 1.8504, 1.91198);

/** Shade the object, applying lighting. */
void main(){

	vec3 worldPos;
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

	} else {
		// For the tessellated grid just use the input info.
		worldPos = In.pos;
		vec3 dView = worldPos - camPos;
		viewDist = length(dView);
		vdir = dView/max(viewDist, 0.001);
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
	vec3 n = normalize(nn);

	// Apply a basic Phong lobe for now.
	float diffuse = max(0.0, dot(lightDirection, n));
	vec3 ldir = reflect(vdir, n);
	float specular = pow(max(0.0, dot(ldir, lightDirection)), 1024.0);
	vec3 baseColor = vec3(0.02, 0.1, 0.2);

	vec3 color = sunColor * (specular + (diffuse + 0.01) * baseColor);
	if(debugCol){
		color = vec3(0.9);
	}
	fragColor = vec4(color,1.0);
}
