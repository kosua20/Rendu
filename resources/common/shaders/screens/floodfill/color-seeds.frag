#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform usampler2D coordsTexture; ///< Seeds coordinates map.
layout(binding = 1) uniform sampler2D inputTexture; ///< Input color image.

layout(location = 0) out vec4 fragCoords; ///< Color.

/** For each pixel, fetch the closest seed coordinates, use them to 
	query in the input texture and output the corresponding color. */
void main(){
	ivec2 baseCoords = ivec2(floor(gl_FragCoord.xy));
	// Assume the resolution is the same.
	uvec2 seedCoords = texelFetch(coordsTexture, baseCoords, 0).xy;
	//Rescale the seed coords.
	vec2 scaledCoords = vec2(seedCoords.xy)/textureSize(coordsTexture, 0) * textureSize(inputTexture, 0);
	ivec2 finalCoords = ivec2(round(scaledCoords));
	// Assume the resolution is the same.
	fragCoords = texelFetch(inputTexture, finalCoords, 0);
}
