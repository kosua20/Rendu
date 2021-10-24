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
layout(set = 2, binding = 3) uniform texture2D texture3; ///< Anisotropy direction and intensity map.

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
	vec3 anisotropyInfos = texture(sampler2D(texture3, sRepeatLinearLinear), In.uv.xy).rgb;
	// Rotate tangent frame to determine the direction of max anisotropy.
	vec2 localRotation = 2.0 * anisotropyInfos.xy - 1.0;
	vec3 anisoDirection = normalize(localRotation.x * tbn[0] + localRotation.y * tbn[1]);

	// Encode anisotropy frame in the G-buffer, in a 10bits and a 2bits components. 
	// Based on the method described by S. Lagarde and E. Golubev in 
	// "The Road toward Unified Rendering with Unity’s High Definition Render Pipeline", 2015,
	// (http://advances.realtimerendering.com/s2018/index.htm#_9hypxp9ajqi)

	// Build default tangent frame from normal.
	vec3 defaultTangent, defaultBitangent;
	buildFrame(n, defaultTangent, defaultBitangent);
	// Project anisotropy direction into the default frame.
	float cosFrame = dot(anisoDirection, defaultTangent);
	float sinFrame = dot(anisoDirection, defaultBitangent);
	// Store the smallest of the two in magnitude to maximize accuracy.
	float orientation = min(abs(sinFrame), abs(cosFrame)) * sqrt(2.0);
	// Store which of the two was stored using the sign.
	bool sineUsed = abs(sinFrame) < abs(cosFrame);
	orientation *= (sineUsed ? 1.0 : -1.0);
	// Generate normalized value for storage.
	float orientationStorage = clamp(orientation, -1.0, 1.0) * 0.5 + 0.5;
	// Store both signs in another low precision parameter.
	uint signs = ((sinFrame < 0.0) ? 1u : 0u) | ((cosFrame < 0.0) ? 2u : 0u);
	// Generate normalized value for storage.
	float signsStorage = float(signs)/3.0;

	// Store values.
	fragColor.rgb = color.rgb;
	fragColor.a = encodeMaterial(MATERIAL_ANISOTROPIC);
	fragNormal.rg = encodeNormal(n);
	// Anisotropy parameters.
	fragNormal.b = orientationStorage;
	fragNormal.a = signsStorage;

	// Roughness and AO.
	fragEffects.rb = infos.rb;
	// Pack metalness and clear coat factor.
	fragEffects.g = encodeMetalnessAndParameter(infos.g, 0.0f);
	// Anisotropy factor (normalized).
	fragEffects.a = anisotropyInfos.b;
}
