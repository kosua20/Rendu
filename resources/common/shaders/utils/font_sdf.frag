#version 330

/// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

uniform vec4 color; ///< The inner glyph color.
uniform vec4 edgeColor; ///< The outer glyph color.
uniform float edgeWidth = 0.0f;  ///< The outer edge width


layout(binding = 0) uniform sampler2D fontSdfTexture; ///< The font signed-distance-function atlas.
layout(location = 0) out vec4 fragColor; ///< Color.

/** Find the isolines of the corresponding glyph to display it on screen. */
void main(){
	// Flip to [1, -1], where > 0 is outside.
	float fontDistance = 1.0-2.0*texture(fontSdfTexture, In.uv).r;
	if(fontDistance > edgeWidth){
		discard;
	} else if(fontDistance > 0.0){
		fragColor = edgeColor;
	} else {
		fragColor = color;
	}
}
