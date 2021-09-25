#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D screenTexture; ///< Color image.

layout(location = 0) out uvec2 fragCoords; ///< Seeds coordinates.

const int unknownCoord = 65500; ///< Arbitrary high number.

/** For each non-black pixel, consider it as a seed and store its coordinates. */
void main(){
	vec3 col = textureLod(sampler2D(screenTexture, sClampNear), In.uv, 0.0).rgb;
	// If a pixel is black, it's not a seed.
	if(all(equal(col, vec3(0.0)))){
		fragCoords = uvec2(unknownCoord);
		return;
	}
	fragCoords = uvec2(floor(gl_FragCoord.xy));
}
