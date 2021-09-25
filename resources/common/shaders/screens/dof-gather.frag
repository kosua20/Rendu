#include "constants.glsl"
#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D sceneColor; ///< Texture to overlay.
layout(set = 2, binding = 1) uniform texture2D cocDepth; ///< Texture to overlay.

layout(set = 0, binding = 0) uniform UniformBlock {
	vec2 invSize; ///< Pixel shift.
};

layout(location = 0) out vec4 fragColor; ///< Scene color.

// DoF extra parameters:
// maximum radius and scale between steps
#define RADIUS 15.0
#define SCALE 0.75

/** Generate a depth of field effect by gathering in a neighborhood of the current fragment using a spiral pattern
 and accumulating contribution from neighbors based on their circle-of-confusion radius.
 Based on "Bokeh depth of field in a single pass" by Dennis Gustafsson, 2018 (http://tuxedolabs.blogspot.com/2018/05/bokeh-depth-of-field-in-single-pass.html).
 */
void main(){
	// Read center info.
	vec2 centerCoc = textureLod(sampler2D(cocDepth, sClampNear), In.uv, 0.0).rg;
	centerCoc.x = clamp(centerCoc.x, 0.0, RADIUS);
	
	vec3 color = textureLod(sampler2D(sceneColor, sClampLinear), In.uv, 0.0).rgb;
	int sum = 1;
	float size = centerCoc.x;

	float radius = SCALE;
	for(float angle = 0.0; radius < RADIUS; angle += GOLDEN_ANGLE){
		// Compute UV following a spiral.
		vec2 newUV = In.uv + vec2(cos(angle), sin(angle)) * invSize * radius;
		vec2 newCoc = textureLod(sampler2D(cocDepth, sClampNear), newUV, 0.0).rg;
		vec3 newColor = textureLod(sampler2D(sceneColor, sClampLinear), newUV, 0.0).rgb;
		// If the CoC disagrees to much, we are at a background depth discontinuity.
		if(newCoc.y > centerCoc.y){
			newCoc.x = clamp(newCoc.x, 0.0, 2.0 * centerCoc.x);
		}
		// Is the sample circle large enough to contribute to the current fragment.
		float validity = smoothstep(radius - 0.5, radius + 0.5, newCoc.x);
		// Accumulate, fallback to the average color.
		color += mix(color/sum, newColor, validity);
		size += newCoc.x;
		sum += 1;
		radius += SCALE/radius;
	}

	fragColor = vec4(color, size) / sum;
}
