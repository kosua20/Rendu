#include "utils.glsl"
#include "samplers.glsl"

layout(location = 0) in INTERFACE {
	vec2 uv; ///< Texture coordinates.
} In ;

layout(set = 2, binding = 0) uniform texture2D normalMap; ///< Normal map.
layout(set = 2, binding = 1) uniform texture2D materialMap; ///< Material index.
layout(set = 2, binding = 2) uniform texture2D ssaoMap; ///< SSAO result.
layout(set = 2, binding = 3) uniform textureCube envMap; ///< Environment map.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Apply lighting, ambient occlusion and reflections to each fragment based on material info. */
void main(){
	
	float matId  = texelFetch(sampler2D(materialMap, sClampNear), ivec2(gl_FragCoord.xy), 0).x * 255.0;
	// Normal in world space.
	vec3 n = normalize(2.0f * texelFetch(sampler2D(normalMap, sClampNear), ivec2(gl_FragCoord.xy), 0).rgb - 1.0f);
	
	// Ground is white.
	vec3 baseColor = vec3(1.0);
	if(matId > 1.5){
		// Head is grey, body and items are red.
		baseColor = matId > 4.5  ? vec3(0.1) : (matId > 3.5 ? vec3(0.9) : vec3(0.9, 0.0, 0.0));
		float atten = 1.0-pow(n.z, 16.0);
		// Composite reflection, we fetch in world space.
		baseColor *= mix(vec3(1.0), textureLod(samplerCube(envMap, sClampLinear), toCube(n), 3.0).rgb, atten);
	}
	float light0 = max(0.0, 0.3535*(n.y+n.z) + 0.8);
	// Add AO.
	float ao = texture(sampler2D(ssaoMap, sClampLinear), In.uv).r;
	float adjustedAO = 0.9*ao + 0.1;
	// Combine.
	baseColor *= light0 * adjustedAO;
	fragColor.rgb = baseColor;
	fragColor.a = 1.0f;
}
