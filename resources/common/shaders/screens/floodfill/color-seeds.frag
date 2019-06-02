#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform usampler2D coordsTexture; ///< Image to output.
layout(binding = 1) uniform sampler2D inputTexture; ///< Image to output.

layout(location = 0) out vec4 fragCoords; ///< Color.


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
