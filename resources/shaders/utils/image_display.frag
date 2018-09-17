#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to output.

layout(location = 0) out vec4 fragColor; ///< Color.


/** Just pass the input image as-is, potentially performing up/down scaling. */
void main(){
	vec2 uv = In.uv;
	fragColor = texture(screenTexture, uv);
	
}
