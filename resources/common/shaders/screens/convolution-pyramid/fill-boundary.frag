#version 400

in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to process.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Denotes if a pixel falls outside an image.
 \param pos the pixel position
 \param size the image size
 \return true if the pixel is outside of the image
 */
bool isOutside(ivec2 pos, ivec2 size){
	return (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y);
}

/**  Output color only on the edges of the black regions in the input image, along with a 1.0 alpha. */
void main(){

	fragColor = vec4(0.0);

	vec3 fullColor = textureLod(screenTexture, In.uv, 0.0).rgb;
	
	float isInMask = float(all(equal(fullColor, vec3(0.0))));
	float maskLaplacian = -4.0*isInMask;

	vec3 col110 = textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 1, 0)).rgb;
	vec3 col101 = textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0, 1)).rgb;
	vec3 col010 = textureLodOffset(screenTexture, In.uv, 0.0, ivec2(-1, 0)).rgb;
	vec3 col001 = textureLodOffset(screenTexture, In.uv, 0.0, ivec2( 0,-1)).rgb;
	maskLaplacian += float(all(equal(col110, vec3(0.0))));
	maskLaplacian += float(all(equal(col101, vec3(0.0))));
	maskLaplacian += float(all(equal(col010, vec3(0.0))));
	maskLaplacian += float(all(equal(col001, vec3(0.0))));
	
	if(maskLaplacian > 0.0){
		fragColor.rgb = fullColor;
		fragColor.a = 1.0f;
	}
	
}
