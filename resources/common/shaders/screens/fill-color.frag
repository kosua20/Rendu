#version 400

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

uniform vec4 color; ///< Custom fill color.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Fill the screen with a fixed color. */
void main(){
	fragColor = color;
}
