#include "samplers.glsl"
#include "materials.glsl"
#include "utils.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D texture0; ///< Emissive.

layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec4 fragNormal; ///< View space normal.
layout (location = 2) out vec4 fragEffects; ///< Effects.

/** Transfer emissive. */
void main(){
	
	vec4 color = texture(sampler2D(texture0, sRepeatLinearLinear), In.uv);
	if(color.a <= 0.01){
		discard;
	}
	
	// Store values.
	// Normalize emissive color, store intensity in effects texture.
	float m = max(max(color.r, 0.001), max(color.g, color.b));
	fragColor.rgb = color.rgb / m;
	fragColor.a = encodeMaterial(MATERIAL_EMISSIVE);
	fragNormal.rgb = vec3(0.5);
	fragNormal.a = 0.0;
	fragEffects = floatToVec4(m);
}
