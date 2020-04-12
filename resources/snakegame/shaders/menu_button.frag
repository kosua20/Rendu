#version 400

uniform vec4 color; ///< The button color.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Apply the button color. */
void main(){
	fragColor = color;
}
