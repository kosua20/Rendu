#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

// Uniforms: the textures.
layout(binding = 0) uniform sampler2D texture0; ///< Blur level.
layout(binding = 1) uniform sampler2D texture1; ///< Blur level.

layout(location = 0) out vec3 fragColor; ///< Color.

/** Merge multiple versions of the same image, blurred at different scales. */
void main(){
	vec3 col = texture(texture0, In.uv).rgb;
	col += texture(texture1, In.uv).rgb;
	fragColor = col / 2.0;
}
