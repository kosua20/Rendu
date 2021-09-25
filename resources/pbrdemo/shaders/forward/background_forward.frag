#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In;

layout(set = 2, binding = 0) uniform texture2D texture0; ///< Image.

layout(set = 0, binding = 0) uniform UniformBlock {
	vec3 bgColor; ///< Background color.
	bool useTexture; ///< Should the texture be used instead of the color.
};

layout(location = 0) out vec4 fragColor; ///< Color.

/** Transfer color. */
void main(){

	fragColor.rgb = useTexture ? textureLod(sampler2D(texture0, sRepeatLinear), In.uv, 0.0).rgb : bgColor;
	fragColor.a = -1.0;

}
