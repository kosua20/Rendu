#version 330
/// Input: texture coordinates.
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

uniform bool hasMask = false; ///< Should the object alpha mask be applied.
layout(binding = 0) uniform sampler2D mask; ///< RGBA texture.

layout(location = 0) out float fragColor; ///< Depth.

/** Output the final depth of the fragment. */
void main(){
	if(hasMask){
		float a = texture(mask, In.uv).a;
		if(a <= 0.01){
			discard;
		}
	}
	fragColor = gl_FragCoord.z;
	
}
