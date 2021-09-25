#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 0, binding = 0) uniform UniformBlock {
	vec4 color; ///< The inner glyph color.
	vec4 edgeColor; ///< The outer glyph color.
	float edgeWidth;  ///< The outer edge width
};

layout(set = 2, binding = 0) uniform texture2D fontSdfTexture; ///< The font signed-distance-function atlas.
layout(location = 0) out vec4 fragColor; ///< Color.

/** Find the isolines of the corresponding glyph to display it on screen. */
void main(){
	// Flip to [1, -1], where > 0 is outside.
	float fontDistance = 1.0 - 2.0 * texture(sampler2D(fontSdfTexture, sClampLinearLinear), In.uv).r;
	if(fontDistance > edgeWidth){
		discard;
	} else if(fontDistance > 0.0){
		fragColor = edgeColor;
	} else {
		fragColor = color;
	}
}
