#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In;

layout(set = 1, binding = 0) uniform texture2D screenTexture; ///< Level to filter.

layout(location = 0) out vec4 fragColor; ///< Color.

layout(set = 0, binding = 0) uniform UniformBlock {
	float g[3]; ///< g filter parameters.
};

/** Denotes if a pixel falls outside an image.
 \param pos the pixel position
 \param size the image size
 \return true if the pixel is outside of the image
 */
bool isOutside(ivec2 pos, ivec2 size){
	return (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y);
}

/**  Apply the g filter to the input data. */
void main(){
	vec4 accum = vec4(0.0);
	ivec2 size = textureSize(screenTexture, 0).xy;
	ivec2 coords = ivec2(floor(gl_FragCoord.xy));
	for(int dy = -1; dy <=1; dy++){
		for(int dx = -1; dx <=1; dx++){
			ivec2 newPix = coords+ivec2(dx,dy);
			if(isOutside(newPix, size)){
				continue;
			}
			accum += g[dx+1] * g[dy+1] * texelFetch(sampler2D(screenTexture, sClampNear), newPix,0);
		}
	}
	fragColor = accum;
}
