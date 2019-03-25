#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

uniform vec4 color;
uniform vec4 edgeColor;
uniform float edgeWidth = 0.0f;


layout(binding = 0) uniform sampler2D fontSdfTexture; ///< Image to display.
layout(location = 0) out vec4 fragColor; ///< Color.


void main(){
	// Flip to [1, -1], where > 0 is outside.
	float fontDistance = 1.0-2.0*texture(fontSdfTexture, In.uv).r;
	if(fontDistance > edgeWidth){
		discard;
	}
	
	if(fontDistance > 0.0){
		fragColor = edgeColor;
	} else {
		fragColor = color;
	}
}
