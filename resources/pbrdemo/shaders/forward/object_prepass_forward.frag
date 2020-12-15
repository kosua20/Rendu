

in INTERFACE {
    vec3 n; ///< Normal direction.
    vec2 uv; ///< Texture coordinates.
} In ;

layout(binding = 0) uniform sampler2D mask;  ///< RGBA texture.
uniform bool hasMask = false; ///< Alpha masking applied. 

layout (location = 0) out vec4 fragNormal; ///< Geometric normal.

/** Just output the interpolated normal. */
void main(){
	
	// Mask cutout.
	if(hasMask){
		float a = texture(mask, In.uv).a;
		if(a <= 0.01){
			discard;
		}
	}
	
	// Flip the up of the local frame for back facing fragments.
	vec3 n = In.n;
	n *= (gl_FrontFacing ? 1.0 : -1.0);
	fragNormal= vec4(normalize(n) * 0.5 + 0.5, 1.0);

}
