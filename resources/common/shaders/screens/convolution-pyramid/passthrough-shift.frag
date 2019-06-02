#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to output.

layout(location = 0) out vec4 fragColor; ///< Color.

uniform int padding = 0;

/** Just pass the input image as-is, without any resizing. */
void main(){
	
	fragColor = texelFetch(screenTexture,ivec2(floor(gl_FragCoord.xy) - padding),0);
	
}
