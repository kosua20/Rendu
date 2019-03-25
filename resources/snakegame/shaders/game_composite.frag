#version 330

// Input: UV coordinates
in INTERFACE {
	vec2 uv;
} In ; ///< vec2 uv;

layout(binding = 0) uniform sampler2D normalMap; ///< Normal map.
layout(binding = 1) uniform sampler2D materialMap; ///< Material index.
layout(binding = 2) uniform sampler2D ssaoMap; ///< SSAO result.
layout(binding = 3) uniform samplerCube envMap; ///< Environment map.

layout(location = 0) out vec4 fragColor; ///< Color.

/** Apply lighting, ambient occlusion and reflections to each fragment based on material info. */
void main(){
	
	float matId  = texelFetch(materialMap, ivec2(gl_FragCoord.xy), 0).x * 255.0;
	// Normal in world space.
	vec3 n = normalize(2.0f * texelFetch(normalMap, ivec2(gl_FragCoord.xy), 0).rgb - 1.0f);
	
	// Ground is white.
	vec3 baseColor = vec3(1.0);
	if(matId > 1.5){
		// Head is grey, body and items are red.
		baseColor = matId > 4.5  ? vec3(0.1) : (matId > 3.5 ? vec3(0.9) : vec3(0.9, 0.0, 0.0));
		float atten = 1.0-pow(n.z, 16.0);
		// Composite reflection, we fetch in world space.
		baseColor *= mix(vec3(1.0), textureLod(envMap, n, 3.0).rgb, atten);
	}
	float light0 = max(0.0, 0.3535*(n.y+n.z) + 0.8);
	// Add AO.
	float ao = texture(ssaoMap, In.uv).r;
	float adjustedAO = 0.7*ao + 0.3;
	// Combine.
	baseColor *= light0 * adjustedAO;
	fragColor.rgb = baseColor;
	fragColor.a = 1.0f;
}
