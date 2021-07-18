
layout(location = 0) in INTERFACE {
	vec3 col; ///< Interpolated vertex color.
	vec2 uv;
} In ;



layout(binding = 1) uniform UniformBlock {
	vec3 color;
};

layout(location = 0) out vec4 fragColor; ///< Color.

layout(set = 1, binding = 0) uniform sampler2D tex;
/** Render a mesh with its colors. */
void main(){
	fragColor.rgb = texture(tex, In.uv).rgb;
	fragColor.a = 1.0;

}
