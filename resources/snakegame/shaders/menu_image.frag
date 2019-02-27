#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D imageTexture; ///< Image to display.
layout(location = 0) out vec4 fragColor; ///< Color.


void main(){
	
	fragColor = texture(imageTexture, In.uv);
	
}
