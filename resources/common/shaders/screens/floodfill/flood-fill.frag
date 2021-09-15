#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 1, binding = 0) uniform utexture2D screenTexture; ///< Current seed map.

layout(set = 0, binding = 0) uniform UniformBlock {
	int stepDist; ///< The distance between samples.
};

layout(location = 0) out uvec2 fragCoords; ///< Closest seed coordinates.

/** Denotes if a pixel falls outside an image.
 \param pos the pixel position
 \param size the image size
 \return true if the pixel is outside of the image
 */
bool isOutside(ivec2 pos, ivec2 size){
	return (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y);
}

/** Look at the closest seed for a set of neighbours, and keep the closest one for the current pixel. */
void main(){
	
	vec2 currentPixel = vec2(floor(gl_FragCoord.xy));
	ivec2 baseCoords = ivec2(currentPixel);
	ivec2 size = ivec2(textureSize(screenTexture, 0));
	
	// Fetch the current seed for the pixel.
	uvec2 bestSeed = texelFetch(usampler2D(screenTexture, sRepeatNear), baseCoords, 0).xy;
	float bestDist = length(vec2(bestSeed) - currentPixel);
	
	// Fetch the 8 neighbours, at a distance stepDist, and find the closest one.
	int d = stepDist;
	ivec2 deltas[8] = ivec2[8](ivec2(-d,-d), ivec2(-d, 0), ivec2(-d, d),
							   ivec2( 0,-d), 			   ivec2( 0, d),
							   ivec2( d,-d), ivec2( d, 0), ivec2( d, d));
	for(int i = 0; i < 8; ++i){
		ivec2 coords = baseCoords + deltas[i];
		if(isOutside(coords, size)){
			continue;
		}
		uvec2 seed = texelFetch(usampler2D(screenTexture, sRepeatNear), coords, 0).xy;
		float dist = length(vec2(seed) - currentPixel);
		if(dist <= bestDist){
			bestDist = dist;
			bestSeed = seed;
		}
	}
	fragCoords = bestSeed;
}
