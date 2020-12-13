
in INTERFACE {
	vec3 col; ///< Interpolated vertex color.
} In ;

layout(location = 0) out vec4 fragColor; ///< Color.

/** Render a mesh with its colors. */
void main(){
	fragColor.rgb = In.col;
	fragColor.a = 1.0;
}
