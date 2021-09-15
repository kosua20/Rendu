#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 1, binding = 0) uniform texture2D screenTexture; ///< Filled image.
layout(set = 1, binding = 1) uniform texture2D inputTexture; ///< Initial image.

layout(location = 0) out vec4 fragColor; ///< Color.


/** Composite the initial image and the filled image in the regions where the initial image is black. */
void main(){

	vec3 inputColor = textureLod(sampler2D(inputTexture, sClampLinear), In.uv, 0.0).rgb;
	float mask = float(all(equal(inputColor, vec3(0.0))));
	
	vec4 fillColor = textureLod(sampler2D(screenTexture, sClampLinear), In.uv, 0.0);
	fillColor.rgb /= fillColor.a;
	
	fragColor.rgb = mix(inputColor, fillColor.rgb, mask);
	fragColor.a = 1.0;
}
