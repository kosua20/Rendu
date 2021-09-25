#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D screenTexture; ///< Lighting buffer to filter.

layout(set = 0, binding = 0) uniform UniformBlock {
	float luminanceTh; ///< Luminance minimum threshold.
};

layout(location = 0) out vec3 fragColor; ///< Scene color.

/** Extract the areas of the image where the luminance is higher than 1.0. */
void main(){
	fragColor = vec3(0.0);
	
	vec3 color = textureLod(sampler2D(screenTexture, sClampLinear), In.uv, 0.0).rgb;
	// Compute intensity (luminance). If > 1.0, bloom should be visible.
	if(dot(color, vec3(0.289, 0.527, 0.184)) > luminanceTh){
		fragColor = color;
	}
}
