#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D screenTexture; ///< Filled image.
layout(binding = 1) uniform sampler2D inputTexture; ///< Initial image.

layout(location = 0) out vec4 fragColor; ///< Color.


/** Composite the initial image and the filled image in the regions where the initial image is black. */
void main(){
	
	vec3 inputColor = texture(inputTexture, In.uv, -1000.0).rgb;
	float mask = float(all(equal(inputColor, vec3(0.0))));
	
	vec4 fillColor = texture(screenTexture, In.uv, -1000.0);
	fillColor.rgb /= fillColor.a;
	
	fragColor.rgb = mix(inputColor, fillColor.rgb, mask);
	fragColor.a = 1.0;
}
