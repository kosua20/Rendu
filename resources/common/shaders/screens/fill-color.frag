
layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 0, binding = 0) uniform UniformBlock {
	vec4 color; ///< Custom fill color.
};

layout(location = 0) out vec4 fragColor; ///< Color.

/** Fill the screen with a fixed color. */
void main(){
	fragColor = color;
}
