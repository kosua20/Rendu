
in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In;

layout(binding = 0) uniform sampler2D texture0; ///< Image.
uniform vec3 bgColor = vec3(0.0); ///< Background color.
uniform bool useTexture = false; ///< Should the texture be used instead of the color.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Transfer color. */
void main(){

	fragColor.rgb = useTexture ? textureLod(texture0, In.uv, 0.0).rgb : bgColor;
	fragColor.a = -1.0;

}
