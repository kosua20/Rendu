#version 400

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(binding = 0) uniform sampler2D fullResSharp; ///< Full res in-focus image.
layout(binding = 1) uniform sampler2D halfResBlur; ///< Half-res defocused image.

layout(location = 0) out vec4 fragColor; ///< Scene color.

/** Mix a half resolution blurred image with a high resolution image, based on the average circle of confidence size
used when generating the blurred image. */
void main(){

	vec4 blurColor = textureLod(halfResBlur, In.uv, 0.0);
	vec3 sharpCol = textureLod(fullResSharp, In.uv, 0.0).rgb;
	// If the avg CoC size was below 1.0, we use the sharp image. After that we transition smoothly.
	const float cocTh = 5.0;
	float blendFactor = clamp((blurColor.a - 1.0)/cocTh, 0.0, 1.0);
	fragColor.rgb = mix(sharpCol, blurColor.rgb, blendFactor);
	fragColor.a = 1.0;
}
