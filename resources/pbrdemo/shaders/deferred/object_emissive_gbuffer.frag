#include "samplers.glsl"
#include "materials.glsl"
#include "utils.glsl"

layout(location = 0) in INTERFACE {
    mat4 tbn; ///< Normal to view matrix.
	vec4 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D texture0; ///< Emissive.
layout(set = 2, binding = 1) uniform texture2D texture1; ///< Normal map.
layout(set = 2, binding = 2) uniform texture2D texture2; ///< Roughness map.

layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec4 fragNormal; ///< View space normal.
layout (location = 2) out vec4 fragEffects; ///< Effects.

layout(set = 0, binding = 0) uniform UniformBlock {
	bool hasUV; ///< Does the mesh have texture coordinates.
};

/** Transfer emissive along with the material ID, normals and roughness info. */
void main(){
	
	vec4 color = texture(sampler2D(texture0, sRepeatLinearLinear), In.uv.xy);
	if(color.a <= 0.01){
		discard;
	}

	// Flip the up of the local frame for back facing fragments.
	mat3 tbn = mat3(In.tbn);
	tbn[2] *= (gl_FrontFacing ? 1.0 : -1.0);
	// Compute the normal at the fragment using the tangent space matrix and the normal read in the normal map.
	vec3 n;
	if(hasUV){
		n = texture(sampler2D(texture1, sRepeatLinearLinear), In.uv.xy).rgb;
		n = normalize(n * 2.0 - 1.0);
		n = normalize(tbn * n);
	} else {
		n = normalize(tbn[2]);
	}

	float roughness = texture(sampler2D(texture2, sRepeatLinearLinear), In.uv.xy).r;
	
	// Store values.
	// Normalize emissive color, store intensity in effects texture.
	float m = max(max(color.r, 0.001), max(color.g, color.b));
	fragColor.rgb = color.rgb / m;
	fragColor.a = encodeMaterial(MATERIAL_EMISSIVE);
	// Store encoded normal.
	fragNormal.rg = encodeNormal(n);
	// Store roughness
	fragNormal.b = roughness;
	fragNormal.a = 0.0;

	fragEffects = floatToVec4(m);
}
