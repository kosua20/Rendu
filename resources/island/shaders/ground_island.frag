#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec4 pos; ///< World position
	vec2 uv; ///< Texture coordinates
} In ;

layout(set = 0, binding = 0) uniform UniformBlock {
	vec3 lightDirection; ///< Sun light direction.
	bool debugCol; ///< Use debug color instead of shading.
	vec3 camPos; ///< Camera world position.
};

layout(set = 1, binding = 0) uniform texture2D heightMap; ///< Terrain height map, height in R, normals in GBA.
layout(set = 1, binding = 1) uniform texture2D shadowMap; ///<Terrain shadowing factor, ground level in R, water level in G.
layout(set = 1, binding = 2) uniform texture2D surfaceNoise; ///< Noise surface normal map.
layout(set = 1, binding = 3) uniform texture2D glitterNoise; ///< Noise specular map.
layout(set = 1, binding = 4) uniform texture2D sandMapSteep; ///< Normal map for steep dunes.
layout(set = 1, binding = 5) uniform texture2D sandMapFlat; ///< Normal map for flat regions.

layout (location = 0) out vec4 fragColor; ///< Terrain appearance.
layout (location = 1) out vec4 fragWorldPos; ///< Terrain world position.

/** Shade the terrain by simulating sand appearance on dunes. */
void main(){

	fragWorldPos.xyz = In.pos.xyz;
	fragWorldPos.w = 1.0;

	// Get clean normal and height.
	vec4 heightAndNor = textureLod(sampler2D(heightMap, sClampLinear), In.uv, 0.0);
	vec3 n = normalize(heightAndNor.yzw);
	vec3 v = normalize(camPos - fragWorldPos.xyz);

	// Transition weight between planar and steep regions, for both X and Z orientations.
	float wFlat = pow(abs(n.y), 8.0);
	float wXdir = abs(n.y) > 0.99 ? 0.0 : (abs(n.x)/sqrt(1.0-n.y*n.y));
	vec3 blend = vec3(wXdir*(1.0-wFlat), wFlat, (1.0-wXdir)*(1.0-wFlat));

	// Sand normal map, blended.
	vec3 mapsUV = 3.0*fragWorldPos.xyz;
	vec2 mapsUvX = mapsUV.yz;
	vec2 mapsUvY = mapsUV.zx;
	vec2 mapsUvZ = mapsUV.yx;

	vec3 nSandX = normalize(texture(sampler2D(sandMapSteep, sRepeatLinearLinear), mapsUvX).rgb * 2.0 - 1.0);
	vec3 nSandZ = normalize(texture(sampler2D(sandMapSteep, sRepeatLinearLinear), mapsUvZ).rgb * 2.0 - 1.0);
	vec3 nSandY = normalize(texture(sampler2D(sandMapFlat, sRepeatLinearLinear), mapsUvY).rgb * 2.0 - 1.0);

	nSandX = vec3(nSandX.xy + n.zy, abs(nSandX.z) * n.x);
	nSandY = vec3(nSandY.xy + n.xz, abs(nSandY.z) * n.y);
	nSandZ = vec3(nSandZ.xy + n.xy, abs(nSandZ.z) * n.z);

	vec3 nMap = normalize(nSandX.zyx * blend.x + nSandY.xzy * blend.y + nSandZ.xyz * blend.z);

	// Add extra perturbation.
	vec3 surfN = normalize(texture(sampler2D(surfaceNoise, sRepeatLinearLinear), 200.0*In.uv).rgb);
	vec3 finalN = normalize(vec3(nMap.xy + surfN.xy, nMap.z*surfN.z));

	// Shadow
	float shadow = textureLod(sampler2D(shadowMap, sClampLinear), In.uv, 0.0).r;
	
	// Colors.
	float colorBlend = texture(sampler2D(surfaceNoise, sRepeatLinearLinear), 150.0*(In.uv + vec2(0.73, 0.19))).a;
	vec3 sandColorDark = vec3(0.3, 0.22, 0.15);
	vec3 sandColorLight = vec3(1.0, 0.516, 0.188);
	vec3 sandColor = mix(sandColorLight, sandColorDark, colorBlend);
	vec3 shadowColor = 0.2 * sandColor;
	vec3 specColor = vec3(1.0, 0.642, 0.378);

	// Tweaked diffuse.
	float diffuse = clamp(2.0 * dot(vec3(1.0,0.3,1.0) * finalN, lightDirection), 0.0, 1.0);
	vec3 color = mix(shadowColor, sandColor, shadow * diffuse);

	// Fresnel.
	float F = pow(1.0 - max(dot(finalN, v), 0.0), 5.0);
	// Specular lobe.
	vec3 h = normalize(v + lightDirection);
	float lobe = pow(max(dot(finalN, h), 0.0), 64.0);
	// Extra glitter
	vec2 glitterUV = 50.0 * In.uv + vec2(0.71, 0.23);
	vec3 glitterN2 = normalize(texture(sampler2D(glitterNoise, sRepeatLinearLinear), glitterUV).rgb);
	vec3 lightBounce = reflect(-lightDirection, glitterN2);
	float glit = smoothstep(0.9, 1.0, dot(lightBounce, v));
	// Total specular.
	vec3 spec = (shadow * 0.9 + 0.1) * (F * specColor + lobe * specColor + glit * specColor);
	
	color += spec;
	
	if(debugCol){
		color = vec3(0.9,0.9,0.9);
	}
	fragColor.rgb = color;
	fragColor.a = 1.0;
}
