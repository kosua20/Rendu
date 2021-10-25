#include "samplers.glsl"
#include "materials.glsl"
#include "utils.glsl"

layout(location = 0) in INTERFACE {
    mat4 tbn; ///< Normal to view matrix.
	vec4 uv; ///< UV coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D texture0; ///< Albedo.
layout(set = 2, binding = 1) uniform texture2D texture1; ///< Normal map.
layout(set = 2, binding = 2) uniform texture2D texture2; ///< Effects map.
layout(set = 2, binding = 3) uniform texture2D texture3; ///< Sheen color and roughness.

layout (location = 0) out vec4 fragColor; ///< Color.
layout (location = 1) out vec4 fragNormal; ///< View space normal.
layout (location = 2) out vec4 fragEffects; ///< Effects.

layout(set = 0, binding = 0) uniform UniformBlock {
	bool hasUV; ///< Does the mesh have texture coordinates.
};

/** Transfer albedo and effects along with the material ID, and output the final normal 
	(combining geometry normal and normal map) in view space. */
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
	
	vec3 infos = texture(sampler2D(texture2, sRepeatLinearLinear), In.uv.xy).rgb;
	vec4 sheenInfos = texture(sampler2D(texture3, sRepeatLinearLinear), In.uv.xy);

	// Store values.
	fragColor.rgb = color.rgb;
	fragColor.a = encodeMaterial(MATERIAL_SHEEN);
	fragNormal.rg = encodeNormal(n);
	// Roughness and AO.
	fragEffects.rb = infos.rb;

	// Sheen parameters.
	// Encode color on 12 bits.
	uvec3 sheenColor = uvec3(sheenInfos.rgb * 255.0);
	sheenColor = (sheenColor & 0xF0) >> 4;
	uint sheenPacked = sheenColor.r | (sheenColor.g << 4) | (sheenColor.b << 8);
	// Split it into 10 and 2 bits for storage along with the normal.
	float sheenPacked10 = float((sheenPacked & 0xFFFFFC) >> 2) / float((1 << 10) - 1);
	float sheenPacked2 = float((sheenPacked & 0x03)) / float((1<<2)-1);
	fragNormal.b = sheenPacked10;
	fragNormal.a = sheenPacked2;
	
	// No metalness, replace it by sheeness.
	fragEffects.g = encodeMetalnessAndParameter(infos.g, 0.0f);
	// Sheen roughness.
	fragEffects.a = sheenInfos.a;
}
