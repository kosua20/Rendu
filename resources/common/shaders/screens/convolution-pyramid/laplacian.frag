#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to process.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Denotes if UV coordinates falls outside an image.
 \param pos the UV coordinates
 \return true if the UV are outside of the image
 */
bool isOutside(vec2 pos){
	return (pos.x < 0.0 || pos.y < 0.0 || pos.x > 1.0 || pos.y > 1.0);
}

uniform int scale = 1; ///< The downscaling applied to the input image.

/** Compute the Laplacian field of an input RGB image, adding a 1px black border around it before computing the gradients and divergence. */
void main(){

	vec3 div = vec3(0.0);
	
	ivec2 size = textureSize(screenTexture, 0).xy;
	ivec2 padSize = size/scale + ivec2(2);
	
	vec3 pixelShift = vec3(0.0);
	pixelShift.xy = 1.0/vec2(size)*scale;
	
	vec2 uvs = In.uv*(vec2(padSize)*pixelShift.xy) - pixelShift.xy;
	if(!isOutside(uvs)){
		vec3 col = texture(screenTexture, uvs, -1000.0).rgb;
		div = 4.0 * col;
	}
	
	vec2 uvs110 = uvs + pixelShift.xz;
	if(!isOutside(uvs110)){
		vec3 col110 = texture(screenTexture, uvs110, -1000.0).rgb;
		div -= col110;
	}
	vec2 uvs101 = uvs + pixelShift.zy;
	if(!isOutside(uvs101)){
		vec3 col110 = texture(screenTexture, uvs101, -1000.0).rgb;
		div -= col110;
	}
	vec2 uvs010 = uvs - pixelShift.xz;
	if(!isOutside(uvs010)){
		vec3 col110 = texture(screenTexture, uvs010, -1000.0).rgb;
		div -= col110;
	}
	vec2 uvs001 = uvs - pixelShift.zy;
	if(!isOutside(uvs001)){
		vec3 col110 = texture(screenTexture, uvs001, -1000.0).rgb;
		div -= col110;
	}
	
	fragColor.rgb = div;
	fragColor.a = 1.0f;
}
