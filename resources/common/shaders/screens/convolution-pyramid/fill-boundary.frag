#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to output.

layout(location = 0) out vec4 fragColor; ///< Color.

bool isOutside(ivec2 pos, ivec2 size){
	return (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y);
}

/**  */
void main(){

	fragColor = vec4(0.0);

	vec3 fullColor = texture(screenTexture, In.uv, -1000.0).rgb;
	
	float isInMask = float(all(equal(fullColor, vec3(0.0))));
	float maskLaplacian = -4.0*isInMask;

	vec3 col110 = textureOffset(screenTexture, In.uv, ivec2( 1, 0), -1000.0).rgb;
	vec3 col101 = textureOffset(screenTexture, In.uv, ivec2( 0, 1), -1000.0).rgb;
	vec3 col010 = textureOffset(screenTexture, In.uv, ivec2(-1, 0), -1000.0).rgb;
	vec3 col001 = textureOffset(screenTexture, In.uv, ivec2( 0,-1), -1000.0).rgb;
	maskLaplacian += float(all(equal(col110, vec3(0.0))));
	maskLaplacian += float(all(equal(col101, vec3(0.0))));
	maskLaplacian += float(all(equal(col010, vec3(0.0))));
	maskLaplacian += float(all(equal(col001, vec3(0.0))));
	
	if(maskLaplacian > 0.0){
		fragColor.rgb = fullColor;
		fragColor.a = 1.0f;
	}
	
}
