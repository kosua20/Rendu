#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to output.

layout(location = 0) out uvec2 fragCoords; ///< Color.

const int unknownCoord = 65500;

/** Just pass the input image as-is, potentially performing up/down scaling. */
void main(){
	vec3 col = texture(screenTexture, In.uv, -1000.0).rgb;
	
	if(all(equal(col, vec3(0.0)))){
		fragCoords = uvec2(unknownCoord);
		return;
	}
	fragCoords = uvec2(floor(gl_FragCoord.xy));
}
