#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to pad.

layout(location = 0) out vec4 fragColor; ///< Color.

uniform int padding = 0; ///< The padding to apply.

/** Ouptut an image translated by a fixed number of pixels on each axis. useful for padding when rendering in a larger framebuffer. */
void main(){
	fragColor = texelFetch(screenTexture,ivec2(floor(gl_FragCoord.xy) - padding),0);
}
