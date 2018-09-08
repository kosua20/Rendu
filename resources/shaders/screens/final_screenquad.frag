#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< The input image.

layout(location = 0) out vec3 fragColor; ///< Final color.

/** Apply a sharpening filter to the image (based on Uncharted 2 presentation). */
void main(){
	
	/// \todo Refine performances and effect.
	vec3 finalColor = texture(screenTexture,In.uv).rgb;
	vec3 down = textureOffset(screenTexture,In.uv,ivec2(0,-1)).rgb;
	vec3 up = textureOffset(screenTexture,In.uv,ivec2(0,1)).rgb;
	vec3 left = textureOffset(screenTexture,In.uv,ivec2(-1,0)).rgb;
	vec3 right = textureOffset(screenTexture,In.uv,ivec2(1,0)).rgb;

	fragColor = clamp(finalColor + 0.4*(4 * finalColor - down - up - left - right),0.0,1.0);
	
}
