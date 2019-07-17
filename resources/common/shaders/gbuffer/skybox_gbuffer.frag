#version 330

#define MATERIAL_ID 0 ///< The material ID.

// Input: position in model space
in INTERFACE {
	vec3 position; 
} In ; ///< vec3 position;

layout(binding = 0) uniform samplerCube texture0; ///< Albedo.

layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec3 fragNormal; ///< View space normal.
layout (location = 2) out vec3 fragEffects; ///< Effects.

/** Transfer albedo along with the material ID, and output a null normal. */
void main(){

	fragColor.rgb = textureLod(texture0, normalize(In.position), 0.0).rgb;
	fragColor.a = MATERIAL_ID;
	fragNormal = vec3(0.5);
	fragEffects = vec3(0.0);

}
