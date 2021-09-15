#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 0, binding = 0) uniform UniformBlock {
	float time; ///< Current time.
};

layout(set = 1, binding = 0) uniform texture2D colorTexture; ///< Color backgroudn input.
layout(set = 1, binding = 1) uniform texture2D posTexture; ///< Position map.
layout(set = 1, binding = 2) uniform texture2D caustics; ///< Caustics map.
layout(set = 1, binding = 3) uniform texture2D normalMap; ///< Water normal map.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Copy the scene, applying moving caustics. */
void main(){
	vec3 fragPos = textureLod(sampler2D(posTexture, sClampLinear), In.uv, 0.0).xyz;
	// Compute world-space based UV coordinates.
	vec2 uvs = fragPos.xz + sin(0.1 * time * vec2(0.1, 0.7));
	// Fetch displacement vector from low-frequency normal map.
	vec2 disp =	texture(sampler2D(normalMap, sRepeatLinearLinear), 0.1*uvs).xy;
	// Fetch caustic intensity.
	float caust = texture(sampler2D(caustics, sRepeatLinearLinear), fragPos.xz + 2.0*sin(disp)).x;
	caust *= caust*caust;
	// Soft transition on the shore edges.
	float edgeScale = 15.0*clamp(-5.0*fragPos.y, 0.0, 1.0);
	// Combine ocean floor color and caustics.
	// Clamp to avoid artefacts when blurring.
	fragColor.rgb = min((1.0 + edgeScale * caust) * textureLod(sampler2D(colorTexture, sClampLinear), In.uv, 0.0).rgb, 5.0);
	fragColor.a = 1.0;
}
