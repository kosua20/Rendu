#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to blur.
uniform vec2 fetchOffset; ///< Texture coordinates offset along the correct axis.

out vec3 fragColor; ///< Blurred color.

/** Performs an approximate 5x5 gaussian blur using two separable passes and three texture fetches in each direction. */
void main(){
	// 3 taps only are required for a 5x5 gaussian blur, thanks to separation into
	// an horizontal and vertical 1D blurs, and to bilinear interpolation.
	vec3 col = texture(screenTexture, In.uv).rgb * 6.0/16.0;
	col += texture(screenTexture, In.uv - fetchOffset).rgb * 5.0/16.0;
	col += texture(screenTexture, In.uv + fetchOffset).rgb * 5.0/16.0;
	fragColor = col;
}
