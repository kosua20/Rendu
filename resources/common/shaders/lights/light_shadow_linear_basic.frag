#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec4 worldPos; ///< World space position.
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D mask;  ///< RGBA texture.

layout(set = 0, binding = 0) uniform UniformBlock {
	vec3 lightPositionWorld; ///< The world space position of the light.
	float lightFarPlane; ///< The light projection matrix far plane.
	bool hasMask; ///< Should the object alpha mask be applied.
};

layout(location = 0) out float fragColor; ///< World space depth.

/** Compute the depth in world space, normalized by the far plane distance */
void main(){
	// Mask cutout.
	if(hasMask){
		float a = texture(sampler2D(mask, sRepeatLinearLinear), In.uv).a;
		if(a <= 0.01){
			discard;
		}
	}
	// We compute the distance in world space (or equivalently view space).
	// We normalize it by the far plane distance to obtain a [0,1] value.
	float dist = clamp(length(In.worldPos.xyz - lightPositionWorld) / lightFarPlane, 0.0, 1.0);
	fragColor = dist;
}
