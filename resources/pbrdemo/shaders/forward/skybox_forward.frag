
in INTERFACE {
	vec3 pos; ///< Position in model space.
} In ;

layout(binding = 0) uniform samplerCube texture0; ///< Cubemap color.

layout(location = 0) out vec4 fragColor; ///< Color.
layout(location = 1) out vec3 fragDirect; ///< Direct lighting.
layout(location = 2) out vec3 fragNormal; ///< Normal.

/** Use the normalized position to read in the cube map. */
void main(){
	fragColor.rgb = texture(texture0, normalize(In.pos)).rgb;
	fragColor.a = -1.0;
	fragDirect = vec3(0.0);
	fragNormal = vec3(0.5);
}
