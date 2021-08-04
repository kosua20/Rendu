
layout(location = 0) in INTERFACE {
	vec3 pos; ///< Position in model space.
} In ;

layout(set = 1, binding = 0) uniform samplerCube texture0; ///< Cubemap color.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Use the normalized position to read in the cube map. */
void main(){
	fragColor = texture(texture0, normalize(In.pos));
}
