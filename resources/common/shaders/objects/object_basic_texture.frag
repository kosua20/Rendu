#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ; 

layout(set = 2, binding = 0) uniform texture2D texture0; ///< Color texture.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Texture each face. */
void main(){
	fragColor = texture(sampler2D(texture0, sRepeatLinearLinear), In.uv);
}
