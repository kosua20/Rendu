layout(set = 0, binding = 0) uniform UniformBlock {
	vec4 color; ///< The button color.
};

layout(location = 0) out vec4 fragColor; ///< Color.

/** Apply the button color. */
void main(){
	fragColor = color;
}
