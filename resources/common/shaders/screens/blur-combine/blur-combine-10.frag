#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

// Uniforms: the textures.
layout(binding = 0) uniform sampler2D texture0; ///< Blur level.
layout(binding = 1) uniform sampler2D texture1; ///< Blur level.
layout(binding = 2) uniform sampler2D texture2; ///< Blur level.
layout(binding = 3) uniform sampler2D texture3; ///< Blur level.
layout(binding = 4) uniform sampler2D texture4; ///< Blur level.
layout(binding = 5) uniform sampler2D texture5; ///< Blur level.
layout(binding = 6) uniform sampler2D texture6; ///< Blur level.
layout(binding = 7) uniform sampler2D texture7; ///< Blur level.
layout(binding = 8) uniform sampler2D texture8; ///< Blur level.
layout(binding = 9) uniform sampler2D texture9; ///< Blur level.

layout(location = 0) out vec3 fragColor; ///< Color.

/** Merge multiple versions of the same image, blurred at different scales. */
void main(){
	vec3 col = textureLod(texture0, In.uv, 0.0).rgb;
	col 	+= textureLod(texture1, In.uv, 0.0).rgb;
	col 	+= textureLod(texture2, In.uv, 0.0).rgb;
	col 	+= textureLod(texture3, In.uv, 0.0).rgb;
	col 	+= textureLod(texture4, In.uv, 0.0).rgb;
	col 	+= textureLod(texture5, In.uv, 0.0).rgb;
	col 	+= textureLod(texture6, In.uv, 0.0).rgb;
	col 	+= textureLod(texture7, In.uv, 0.0).rgb;
	col 	+= textureLod(texture8, In.uv, 0.0).rgb;
	col 	+= textureLod(texture9, In.uv, 0.0).rgb;
	fragColor = col / 10.0;
}
