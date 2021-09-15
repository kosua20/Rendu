#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 1, binding = 0) uniform texture2D screenTexture; ///< Image to pad.

layout(set = 0, binding = 0) uniform UniformBlock {
	int padding; ///< The padding to apply.
};

layout(location = 0) out vec4 fragColor; ///< Color.

/** Denotes if a pixel falls outside an image.
 \param pos the pixel position
 \param size the image size
 \return true if the pixel is outside of the image
 */
bool isOutside(ivec2 pos, ivec2 size){
	return (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y);
}

/** Output an image translated by a fixed number of pixels on each axis. useful for padding when rendering in a larger framebuffer. */
void main(){
	ivec2 coords = ivec2(floor(gl_FragCoord.xy) - padding);
	// Check bounds, as texelFetch with out-of-bound coordinates is undefined behaviour.
	if(isOutside(coords, textureSize(screenTexture, 0))){
		fragColor = vec4(0.0);
		return;
	}
	fragColor = texelFetch(sampler2D(screenTexture, sClampNear), coords,0);
}
