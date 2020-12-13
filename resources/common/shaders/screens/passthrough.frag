
in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to output.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Just pass the input image as-is, potentially performing up/down scaling. */
void main(){
	
	fragColor.rgb = textureLod(screenTexture, In.uv, 0.0).rgb;
	fragColor.a = 1.0f;
}
