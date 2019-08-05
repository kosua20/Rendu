#version 330
// Input: texture coordinates.
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

uniform bool hasMask = false; ///< Should the object alpha mask be applied.
layout(binding = 0) uniform sampler2D mask; ///< RGBA texture.

layout(location = 0) out vec2 fragColor; ///< Depth and depth squared.

/** Output the final depth of the fragment and its square, for variance shadow mapping. */
void main(){
	if(hasMask){
		float a = texture(mask, In.uv).a;
		if(a <= 0.01){
			discard;
		}
	}
	fragColor = vec2(gl_FragCoord.z,gl_FragCoord.z*gl_FragCoord.z);
	
}
