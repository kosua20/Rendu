#include "utils.glsl"
#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D colorScene; ///< Scene color.
layout(set = 2, binding = 1) uniform texture2D depthScene; ///< Scene depth.

layout(set = 0, binding = 0) uniform UniformBlock {
	vec2 projParams; ///< Projection depth-related coefficients.
	float focusDist; ///< Distance to the focal plane (in view space).
	float focusScale; ///< Inverse width of the in-focus region.
};

layout(location = 0) out vec3 downscaledColor; ///< Scene color.
layout(location = 1) out vec2 cocDepth; ///< Circle of confusion and depth.


/** Compute view-space depth and a circle of confusion radius, for depth of field. */
void main(){
	// Compute view space depth and circle of confusion.
	float depth = abs(linearizeDepth(textureLod(sampler2D(depthScene, sClampLinear), In.uv, 0.0).r, projParams));
	float coc = focusScale * abs(1.0 / focusDist - 1.0 / depth);
	cocDepth = vec2(coc, depth);
	// Downscale color.
	downscaledColor = textureLod(sampler2D(colorScene, sClampLinear), In.uv, 0.0).rgb;
}
