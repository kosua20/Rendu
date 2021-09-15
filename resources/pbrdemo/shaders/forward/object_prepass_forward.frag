#include "samplers.glsl"

layout(location = 0) in INTERFACE {
    vec4 n; ///< Normal direction.
    vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 1, binding = 0) uniform texture2D mask;  ///< RGBA texture.

layout(set = 0, binding = 0) uniform UniformBlock {
	bool hasMask; ///< Alpha masking applied.
};

layout (location = 0) out vec4 fragNormal; ///< Geometric normal.

/** Just output the interpolated normal. */
void main(){
	
	// Mask cutout.
	if(hasMask){
		float a = texture(sampler2D(mask, sRepeatLinearLinear), In.uv).a;
		if(a <= 0.01){
			discard;
		}
	}
	
	// Flip the up of the local frame for back facing fragments.
	vec3 n = In.n.xyz;
	n *= (gl_FrontFacing ? 1.0 : -1.0);
	fragNormal= vec4(normalize(n) * 0.5 + 0.5, 1.0);

}
