#include "utils.glsl"
#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec3 pos; ///< Position in model space.
} In ;

layout(set = 1, binding = 0) uniform textureCube texture0; ///< Cubemap color.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Use the normalized position to read in the cube map. */
void main(){
	fragColor.rgb = textureLod(samplerCube(texture0, sClampLinear), toCube(normalize(In.pos)), 0.0).rgb;
	fragColor.a = -1.0;
}
