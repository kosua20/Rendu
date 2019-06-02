#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D unfilteredCurrent; ///< Image to output.
layout(binding = 1) uniform sampler2D filteredSmaller; ///< Image to output.

layout(location = 0) out vec4 fragColor; ///< Color.

uniform float h1[5];
uniform float h2;
uniform float g[3];

bool isOutside(ivec2 pos, ivec2 size){
	return (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y);
}

void main(){
	vec4 accum = vec4(0.0);
	ivec2 coords = ivec2(floor(gl_FragCoord.xy));
	ivec2 size = textureSize(unfilteredCurrent, 0).xy;
	
	for(int dy = -1; dy <=1; dy++){
		for(int dx = -1; dx <=1; dx++){
			ivec2 newPix = coords+ivec2(dx,dy);
			if(isOutside(newPix, size)){
				continue;
			}
			accum += g[dx+1] * g[dy+1] * texelFetch(unfilteredCurrent, newPix,0);
		}
	}
	
	ivec2 sizeSmall = textureSize(filteredSmaller, 0).xy;
	
	for(int dy = -2; dy <=2; dy++){
		for(int dx = -2; dx <=2; dx++){
			ivec2 newPix = coords+ivec2(dx,dy);
			// The filter is applied to a texture upscaled by inserting zeros.
			if(newPix.x%2 != 0 || newPix.y%2 != 0){
				continue;
			}
			newPix /= 2;
			newPix += 5;
			if(isOutside(newPix, sizeSmall)){
				continue;
			}
			accum += h2 * h1[dx+2] * h1[dy+2] * texelFetch(filteredSmaller, newPix, 0);
		}
	}
	
	fragColor = accum;
}
