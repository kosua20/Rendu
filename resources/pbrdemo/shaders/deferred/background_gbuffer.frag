#include "samplers.glsl"

#define MATERIAL_ID 0 ///< The material ID.

layout(location = 0) in INTERFACE {
	vec2 uv;  ///< Texture coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D texture0; ///< Image.

layout(set = 0, binding = 0) uniform UniformBlock {
	vec3 bgColor; ///< Background color.
	bool useTexture; ///< Should the texture be used instead of the color.
};

layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec3 fragNormal; ///< View space normal.
layout (location = 2) out vec3 fragEffects; ///< Effects.

/** Transfer color along with the material ID, and output a null normal. */
void main(){

	fragColor.rgb = useTexture ? textureLod(sampler2D(texture0, sClampLinear), In.uv, 0.0).rgb : bgColor;
	fragColor.a = MATERIAL_ID;
	fragNormal = vec3(0.5);
	fragEffects = vec3(0.0);

}
