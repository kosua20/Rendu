#version 330

#define MATERIAL_ID 0 ///< The material ID.

/// Input: uv coordinates.
in INTERFACE {
	vec2 uv; 
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D texture0; ///< Image.
uniform vec3 bgColor = vec3(0.0); ///< Background color.
uniform bool useTexture = false; ///< Should the texture be used instead of the color.

layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec3 fragNormal; ///< View space normal.
layout (location = 2) out vec3 fragEffects; ///< Effects.

/** Transfer color along with the material ID, and output a null normal. */
void main(){

	fragColor.rgb = useTexture ? textureLod(texture0, In.uv, 0.0).rgb : bgColor;
	fragColor.a = MATERIAL_ID;
	fragNormal = vec3(0.5);
	fragEffects = vec3(0.0);

}
