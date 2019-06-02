#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Image to output.
layout(binding = 1) uniform sampler2D inputTexture; ///< Image to output.

layout(location = 0) out vec4 fragColor; ///< Color.


/** Just pass the input image as-is, without any resizing. */
void main(){
	
	vec3 inputColor = texture(inputTexture, In.uv, -1000.0).rgb;
	float mask = float(all(equal(inputColor, vec3(0.0))));
	
	vec4 fillColor = texture(screenTexture, In.uv, -1000.0);
	fillColor.rgb /= fillColor.a;
	
	fragColor.rgb = mix(inputColor, fillColor.rgb, mask);
	fragColor.a = 1.0;
}
