#version 400

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(binding = 0) uniform sampler2D colorTexture; ///< Color to output.
layout(binding = 1) uniform sampler2D posTexture; ///< Position to output.

layout(location = 0) out vec3 fragColor; ///< Color.
layout(location = 1) out vec3 fragPos; ///< Position.

/** Just pass the input image as-is, without any resizing. */
void main(){
	fragColor = textureLod(colorTexture, In.uv, 0.0).rgb;
	fragPos = textureLod(posTexture, In.uv, 0.0).rgb;
}
