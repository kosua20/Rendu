#include "samplers.glsl"
#include "materials.glsl"
#include "utils.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv;  ///< Texture coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D texture0; ///< Image.

layout(set = 0, binding = 0) uniform UniformBlock {
	vec3 bgColor; ///< Background color.
	bool useTexture; ///< Should the texture be used instead of the color.
};

layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec4 fragNormal; ///< View space normal.
layout (location = 2) out vec4 fragEffects; ///< Effects.

/** Transfer color along with the material ID, and output a null normal. */
void main(){

	vec3 color = useTexture ? textureLod(sampler2D(texture0, sClampLinear), In.uv, 0.0).rgb : bgColor;
	// Normalize emissive color, store intensity in effects texture.
	float m = max(max(color.r, 0.001), max(color.g, color.b));
	fragColor.rgb = color / m;
	fragColor.a = encodeMaterial(MATERIAL_UNLIT);
	fragNormal.rg = encodeNormal(vec3(0.0));
	fragNormal.ba = vec2(0.0);
	fragEffects = floatToVec4(m);

}
