#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D mask; ///< RGBA texture.

layout(set = 0, binding = 0) uniform UniformBlock {
	bool hasMask; ///< Should the object alpha mask be applied.
};

layout(location = 0) out vec2 fragColor; ///< Depth and depth squared.

/** Output the final depth of the fragment and its square, for variance shadow mapping. */
void main(){
	if(hasMask){
		float a = texture(sampler2D(mask, sRepeatLinearLinear), In.uv).a;
		if(a <= 0.01){
			discard;
		}
	}
	fragColor = vec2(gl_FragCoord.z,gl_FragCoord.z*gl_FragCoord.z);
	
}
