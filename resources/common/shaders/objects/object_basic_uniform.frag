
uniform vec4 color = vec4(1.0,0.0,0.0,1.0); ///< User-picked color.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Color each face with a uniform color. */
void main(){
	fragColor = color;
}
