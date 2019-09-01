#version 330

/// Interpolated vertex color.
in INTERFACE {
	vec3 col;
} In ; ///< vec3 col;

layout(location = 0) out vec4 fragColor; ///< Color.

/** Render a mesh with its colors. */
void main(){
	fragColor.rgb = In.col;
	fragColor.a = 1.0;
}
