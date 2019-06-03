#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Color image.

layout(location = 0) out uvec2 fragCoords; ///< Seeds coordinates.

const int unknownCoord = 65500; // Arbitrary high number.

/** For each non-black pixel, consider it as a seed and store its coordinates. */
void main(){
	vec3 col = texture(screenTexture, In.uv, -1000.0).rgb;
	// If a pixel is black, it's not a seed.
	if(all(equal(col, vec3(0.0)))){
		fragCoords = uvec2(unknownCoord);
		return;
	}
	fragCoords = uvec2(floor(gl_FragCoord.xy));
}
