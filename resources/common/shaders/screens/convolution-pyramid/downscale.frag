#version 330

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ; 

layout(binding = 0) uniform sampler2D screenTexture; ///< Input level to filter and downscale.

layout(location = 0) out vec4 fragColor; ///< Color.

uniform float h1[5]; ///< h1 filter parameters.

/** Denotes if a pixel falls outside an image.
 \param pos the pixel position
 \param size the image size
 \return true if the pixel is outside of the image
 */
bool isOutside(ivec2 pos, ivec2 size){
	return (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y);
}

/** Apply the h1 filter and downscale the input data by a factor of 2. */
void main(){
	
	vec4 accum = vec4(0.0);
	
	ivec2 size = textureSize(screenTexture, 0).xy;
	// Our current size is half this one, so we have to scale by 2.
	ivec2 coords = 2*((ivec2(floor(gl_FragCoord.xy)) - 5));
	
	for(int dy = -2; dy <=2; dy++){
		for(int dx = -2; dx <=2; dx++){
			ivec2 newPix = coords+ivec2(dx,dy);
			if(isOutside(newPix, size)){
				continue;
			}
			accum += h1[dx+2] * h1[dy+2] * texelFetch(screenTexture, newPix,0);
		}
	}
	fragColor = accum;
}
