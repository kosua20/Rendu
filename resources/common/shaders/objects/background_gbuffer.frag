#version 330

#define MATERIAL_ID 0 ///< The material ID.

// Input: position in model space
in INTERFACE {
	vec2 uv; 
} In ; ///< vec3 position;

layout(binding = 0) uniform sampler2D texture0; ///< Albedo.
uniform vec3 bgColor = vec3(0.0);
uniform bool useTexture = false;

layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec3 fragNormal; ///< View space normal.
layout (location = 2) out vec3 fragEffects; ///< Effects.

/** Transfer albedo along with the material ID, and output a null normal. */
void main(){

	fragColor.rgb = useTexture ? textureLod(texture0, In.uv, 0.0).rgb : bgColor;
	fragColor.a = MATERIAL_ID;
	fragNormal = vec3(0.5);
	fragEffects = vec3(0.0);

}
