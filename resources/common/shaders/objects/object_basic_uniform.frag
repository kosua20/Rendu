layout(set = 0, binding = 0) uniform UniformBlock {
	vec4 color; ///< User-picked color.
};

layout(location = 0) out vec4 fragColor; ///< Color.

/** Color each face with a uniform color. */
void main(){
	fragColor = color;
}
