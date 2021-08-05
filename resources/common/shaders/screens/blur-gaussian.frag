
layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 1, binding = 0) uniform sampler2D screenTexture; ///< Image to blur.

layout(set = 0, binding = 0) uniform UniformBlock {
	vec2 fetchOffset; ///< Texture coordinates offset along the correct axis.
};

layout(location = 0) out vec3 fragColor; ///< Blurred color.

/** Performs an approximate 5x5 gaussian blur using two separable passes and three texture fetches in each direction. */
void main(){
	// 3 taps only are required for a 5x5 gaussian blur, thanks to separation into
	// an horizontal and vertical 1D blurs, and to bilinear interpolation.
	vec3 col = textureLod(screenTexture, In.uv, 0.0).rgb * 6.0/16.0;
	col += textureLod(screenTexture, In.uv - fetchOffset, 0.0).rgb * 5.0/16.0;
	col += textureLod(screenTexture, In.uv + fetchOffset, 0.0).rgb * 5.0/16.0;
	fragColor = col;
}
