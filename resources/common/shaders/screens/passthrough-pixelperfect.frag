#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 1, binding = 0) uniform texture2D screenTexture; ///< Image to output.

layout(location = 0) out vec3 fragColor; ///< Color.

/** Denotes if a pixel falls outside an image.
 \param pos the pixel position
 \param size the image size
 \return true if the pixel is outside of the image
 */
bool isOutside(ivec2 pos, ivec2 size){
	return (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y);
}

/** Just pass the input image as-is, without any resizing. */
void main(){
	ivec2 coords = ivec2(gl_FragCoord.xy-0.5);
	if(isOutside(coords, textureSize(screenTexture, 0))){
		fragColor = vec3(0.0);
		return;
	}
	fragColor = texelFetch(sampler2D(screenTexture, sClampNear), coords, 0).rgb;

}
