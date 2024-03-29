#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform utexture2D coordsTexture; ///< Seeds coordinates map.

layout(location = 0) out vec4 fragCoords; ///< Distance.

/** For each pixel, fetch the closest seed coordinates, and output the normalized euclidean distance with the current point. */
void main(){
	
	ivec2 baseCoords = ivec2(floor(gl_FragCoord.xy));
	// Assume the resolution is the same.
	uvec2 seedCoords = texelFetch(usampler2D(coordsTexture, sRepeatNear), baseCoords, 0).xy;
	//Rescale the seed coords.
	vec2 scaling = 1.0 / textureSize(coordsTexture, 0);
	vec2 scaledSeedCoords = scaling*vec2(seedCoords.xy);
	vec2 scaledBaseCoords = scaling*vec2(baseCoords.xy);
	// Compute L2 distance and store it.
	float dist = distance(scaledSeedCoords, scaledBaseCoords);
	fragCoords.rgb = vec3(dist);
	fragCoords.a = 1.0;
}
