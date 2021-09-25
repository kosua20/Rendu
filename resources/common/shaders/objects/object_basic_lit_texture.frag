#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec4 vn; ///< World space normal.
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 0, binding = 0) uniform UniformBlock {
	vec3 lightDir; ///< Light direction.
};

layout(set = 2, binding = 0) uniform texture2D texture0; ///< Color texture.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Render a textured mesh with three default lights on each side of the scene. */
void main(){
	vec4 baseColor = texture(sampler2D(texture0, sRepeatLinearLinear), In.uv);
	if(baseColor.a < 0.01){
		discard;
	}

	vec3 nn = normalize(In.vn.xyz);
	// Two basic lights.
	float light0 = 0.8 * max(0.0, dot(nn, lightDir));
	float light1 = 0.6 * max(0.0, dot(nn, -lightDir));
	float light2 = 0.3 * max(0.0, dot(nn, lightDir*vec3(1.0,-1.0,1.0)));
	// Color the lights.
	fragColor.rgb = light0*vec3(0.8,0.9,1.0) + light1*vec3(1.0,0.9,0.8) + light2*vec3(0.8,1.0,0.8);
	fragColor.rgb *= baseColor.rgb;
	fragColor.a = 1.0;
	
}
