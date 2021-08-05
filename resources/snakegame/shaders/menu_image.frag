
layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 1, binding = 0) uniform sampler2D imageTexture; ///< Image to display.
layout(location = 0) out vec4 fragColor; ///< Color.

/** Apply the image. */
void main(){
	fragColor = texture(imageTexture, In.uv);
}
