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

layout(location = 0) out vec3 fragColor; ///< Color.

/** Merge multiple versions of the same image, blurred at different scales. */
void main(){
	vec3 col = texture(texture0, In.uv).rgb;
	col += texture(texture1, In.uv).rgb;
	col += texture(texture2, In.uv).rgb;
	col += texture(texture3, In.uv).rgb;
	col += texture(texture4, In.uv).rgb;
	col += texture(texture5, In.uv).rgb;
	fragColor = col / 6.0;
}
