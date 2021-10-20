#include "utils.glsl"
#include "samplers.glsl"
#include "materials.glsl"

layout(location = 0) in INTERFACE {
	vec3 pos; ///< Position in model space.
} In ;

layout(set = 2, binding = 0) uniform textureCube texture0; ///< Albedo.

layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec4 fragNormal; ///< View space normal.
layout (location = 2) out vec4 fragEffects; ///< Effects.

/** Transfer albedo along with the material ID, and output a null normal. */
void main(){
	vec3 color = textureLod(samplerCube(texture0, sClampLinear), toCube(normalize(In.pos)), 0.0).rgb;
	// Normalize emissive color, store intensity in effects texture.
	float m = max(max(color.r, 0.001), max(color.g, color.b));
	fragColor.rgb = color / m;
	fragColor.a = encodeMaterial(MATERIAL_UNLIT);
	fragNormal.rg = encodeNormal(vec3(0.0));
	fragNormal.ba = vec2(0.0);
	fragEffects = floatToVec4(m);

}
